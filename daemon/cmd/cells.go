// SPDX-License-Identifier: Apache-2.0
// Copyright Authors of Cilium

package cmd

import (
	"log/slog"
	"net/http"

	"github.com/cilium/hive/cell"
	"github.com/cilium/statedb"

	healthApi "github.com/cilium/cilium/api/v1/health/server"
	"github.com/cilium/cilium/api/v1/server"
	"github.com/cilium/cilium/daemon/cmd/cni"
	agentK8s "github.com/cilium/cilium/daemon/k8s"
	"github.com/cilium/cilium/daemon/restapi"
	"github.com/cilium/cilium/pkg/api"
	"github.com/cilium/cilium/pkg/auth"
	"github.com/cilium/cilium/pkg/bgpv1"
	cgroup "github.com/cilium/cilium/pkg/cgroups/manager"
	"github.com/cilium/cilium/pkg/ciliumenvoyconfig"
	"github.com/cilium/cilium/pkg/clustermesh"
	cmtypes "github.com/cilium/cilium/pkg/clustermesh/types"
	"github.com/cilium/cilium/pkg/controller"
	"github.com/cilium/cilium/pkg/crypto/certificatemanager"
	"github.com/cilium/cilium/pkg/datapath"
	"github.com/cilium/cilium/pkg/defaults"
	"github.com/cilium/cilium/pkg/dial"
	"github.com/cilium/cilium/pkg/driftchecker"
	"github.com/cilium/cilium/pkg/dynamicconfig"
	"github.com/cilium/cilium/pkg/dynamiclifecycle"
	"github.com/cilium/cilium/pkg/egressgateway"
	"github.com/cilium/cilium/pkg/endpoint"
	endpointcreator "github.com/cilium/cilium/pkg/endpoint/creator"
	"github.com/cilium/cilium/pkg/endpointcleanup"
	"github.com/cilium/cilium/pkg/endpointmanager"
	"github.com/cilium/cilium/pkg/envoy"
	fqdn "github.com/cilium/cilium/pkg/fqdn/cell"
	"github.com/cilium/cilium/pkg/fqdn/defaultdns"
	"github.com/cilium/cilium/pkg/gops"
	"github.com/cilium/cilium/pkg/health"
	hubble "github.com/cilium/cilium/pkg/hubble/cell"
	identity "github.com/cilium/cilium/pkg/identity/cell"
	ipamcell "github.com/cilium/cilium/pkg/ipam/cell"
	ipcache "github.com/cilium/cilium/pkg/ipcache/cell"
	"github.com/cilium/cilium/pkg/k8s"
	k8sClient "github.com/cilium/cilium/pkg/k8s/client"
	k8sSynced "github.com/cilium/cilium/pkg/k8s/synced"
	"github.com/cilium/cilium/pkg/k8s/watchers"
	"github.com/cilium/cilium/pkg/k8s/watchers/resources"
	"github.com/cilium/cilium/pkg/kvstore/store"
	"github.com/cilium/cilium/pkg/l2announcer"
	loadbalancer_experimental "github.com/cilium/cilium/pkg/loadbalancer/experimental"
	redirectpolicy_experimental "github.com/cilium/cilium/pkg/loadbalancer/experimental/redirectpolicy"
	"github.com/cilium/cilium/pkg/logging/logfields"
	"github.com/cilium/cilium/pkg/maglev"
	"github.com/cilium/cilium/pkg/maps/metricsmap"
	natStats "github.com/cilium/cilium/pkg/maps/nat/stats"
	"github.com/cilium/cilium/pkg/maps/ratelimitmap"
	"github.com/cilium/cilium/pkg/metrics"
	"github.com/cilium/cilium/pkg/metrics/features"
	"github.com/cilium/cilium/pkg/node"
	nodeManager "github.com/cilium/cilium/pkg/node/manager"
	"github.com/cilium/cilium/pkg/nodediscovery"
	"github.com/cilium/cilium/pkg/option"
	policy "github.com/cilium/cilium/pkg/policy/cell"
	policyDirectory "github.com/cilium/cilium/pkg/policy/directory"
	policyK8s "github.com/cilium/cilium/pkg/policy/k8s"
	"github.com/cilium/cilium/pkg/pprof"
	"github.com/cilium/cilium/pkg/proxy"
	"github.com/cilium/cilium/pkg/recorder"
	"github.com/cilium/cilium/pkg/redirectpolicy"
	"github.com/cilium/cilium/pkg/service"
	shell "github.com/cilium/cilium/pkg/shell/server"
	"github.com/cilium/cilium/pkg/signal"
	"github.com/cilium/cilium/pkg/source"
	"github.com/cilium/cilium/pkg/status"
)

var (
	Agent = cell.Module(
		"agent",
		"Cilium Agent",

		Infrastructure,
		ControlPlane,
		datapath.Cell,
	)

	// Infrastructure provides access and services to the outside.
	// A cell should live here instead of ControlPlane if it is not needed by
	// integrations tests, or needs to be mocked.
	Infrastructure = cell.Module(
		"infra",
		"Infrastructure",

		// Register the pprof HTTP handlers, to get runtime profiling data.
		pprof.Cell(pprofConfig),

		// Runs the gops agent, a tool to diagnose Go processes.
		gops.Cell(defaults.EnableGops, defaults.GopsPortAgent),

		// Provides Clientset, API for accessing Kubernetes objects.
		k8sClient.Cell,

		cni.Cell,

		// Provide the modular metrics registry, metric HTTP server and legacy metrics cell.
		metrics.Cell,

		// Provides cilium_datapath_drop/forward Prometheus metrics.
		metricsmap.Cell,

		// Provides cilium_bpf_ratelimit_dropped_total Prometheus metric.
		ratelimitmap.Cell,

		// Provide option.Config via hive so cells can depend on the agent config.
		cell.Provide(func() *option.DaemonConfig { return option.Config }),

		// Cilium API served over UNIX sockets. Accessed by the 'cilium' utility (not cilium-cli).
		server.Cell,
		cell.Invoke(configureAPIServer),

		// Cilium API handlers
		cell.Provide(ciliumAPIHandlers),

		// Processes endpoint deletions that occurred while the agent was down.
		// This starts before the API server as ciliumAPIHandlers() depends on
		// the 'deletionQueue' provided by this cell.
		deletionQueueCell,

		// Store cell provides factory for creating watchStore/syncStore/storeManager
		// useful for synchronizing data from/to kvstore.
		store.Cell,

		// Provide CRD resource names for 'k8sSynced.CRDSyncCell' below.
		cell.Provide(func() k8sSynced.CRDSyncResourceNames { return k8sSynced.AgentCRDResourceNames() }),

		// CRDSyncCell provides a promise that is resolved as soon as CRDs used by the
		// agent have k8sSynced.
		// Allows cells to wait for CRDs before trying to list Cilium resources.
		// This is separate from k8sSynced.Cell as this one needs to be mocked for tests.
		k8sSynced.CRDSyncCell,

		// Shell for inspecting the agent. Listens on the 'shell.sock' UNIX socket.
		shell.Cell,

		// DNSProxy provides the DefaultDNSProxy singleton which is used by different
		// packages.
		defaultdns.Cell,
	)

	// ControlPlane implement the per-node control functions. These are pure
	// business logic and depend on datapath or infrastructure to perform
	// actions. This separation enables non-privileged integration testing of
	// the control-plane.
	ControlPlane = cell.Module(
		"controlplane",
		"Control Plane",

		// LocalNodeStore holds onto the information about the local node and allows
		// observing changes to it.
		node.LocalNodeStoreCell,

		// Provide a newLocalNodeSynchronizer that is invoked when LocalNodeStore is started.
		// This fills in the initial state before it is accessed by other sub-systems.
		// Then, it takes care of keeping selected fields (e.g., labels, annotations)
		// synchronized with the corresponding kubernetes object.
		cell.Provide(newLocalNodeSynchronizer),

		// Controller provides flags and configuration related
		// to Controller management, concurrent control loops
		// which run throughout the system on specified intervals
		controller.Cell,

		// Shared resources provide access to k8s resources as event streams or as
		// read-only stores.
		agentK8s.ResourcesCell,

		// StateDB tables for Kubernetes objects.
		agentK8s.TablesCell,

		// Shared synchronization structures for waiting on K8s resources to
		// be synced
		k8sSynced.Cell,

		// EndpointManager maintains a collection of the locally running endpoints.
		endpointmanager.Cell,

		// EndpointCreator helps creating endpoints
		endpointcreator.Cell,

		// Register the startup procedure to remove stale CiliumEndpoints referencing pods no longer
		// managed by Cilium.
		endpointcleanup.Cell,

		// NodeManager maintains a collection of other nodes in the cluster.
		nodeManager.Cell,

		// Certificate manager provides an API for retrieving secrets and certificate in the form of TLS contexts.
		certificatemanager.Cell,

		// Cilium API specification cell makes the swagger model available for reuse
		server.SpecCell,

		// cilium-health connectivity probe API specification cell makes the swagger model available for reuse
		healthApi.SpecCell,

		// daemonCell wraps the legacy daemon initialization and provides Promise[*Daemon].
		daemonCell,

		// Maglev table computtations
		maglev.Cell,

		// Experimental control-plane for configuring service load-balancing.
		loadbalancer_experimental.Cell,

		// Experimental control-plane implementation for local redirect policies.
		redirectpolicy_experimental.Cell,

		// Service is a datapath service handler. Its main responsibility is to reflect
		// service-related changes into BPF maps used by datapath BPF programs.
		service.Cell,

		// Proxy provides the proxy port allocation and related datapath coordination and
		// makes different L7 proxies (Envoy, DNS proxy) usable to Cilium endpoints through
		// a common Proxy 'redirect' abstraction.
		proxy.Cell,

		// Envoy cell which is the control-plane for the Envoy proxy.
		// It is used to provide support for Ingress, GatewayAPI and L7 network policies (e.g. HTTP).
		envoy.Cell,

		// CiliumEnvoyConfig provides support for the CRD CiliumEnvoyConfig that backs Ingress, Gateway API
		// and L7 loadbalancing.
		ciliumenvoyconfig.Cell,

		// Cilium REST API handlers
		restapi.Cell,

		// The BGP Control Plane which enables various BGP related interop.
		bgpv1.Cell,

		// Brokers datapath signals from signalmap
		signal.Cell,

		// Auth is responsible for authenticating a request if required by a policy.
		auth.Cell,

		// Provides Identity Controlplane (Responsible for allocating & managing security identities)
		identity.Cell,

		// IPCache cell provides IPCache (IP to identity mappings)
		ipcache.Cell,

		// IPAM provides IP address management.
		ipamcell.Cell,

		// Egress Gateway allows originating traffic from specific IPv4 addresses.
		egressgateway.Cell,

		// ServiceCache holds the list of known services correlated with the matching endpoints.
		k8s.ServiceCacheCell,

		// Provides PolicyRepository (List of policy rules)
		policy.Cell,

		// K8s policy resource watcher cell. It depends on the half-initialized daemon which is
		// resolved by newDaemonPromise()
		policyK8s.Cell,

		// Directory policy watcher cell.
		policyDirectory.Cell,

		// ClusterMesh is the Cilium's multicluster implementation.
		cell.Config(cmtypes.DefaultClusterInfo),
		clustermesh.Cell,

		// L2announcer resolves l2announcement policies, services, node labels and devices into a list of IPs+netdevs
		// which need to be announced on the local network.
		l2announcer.Cell,

		// RegeneratorCell provides extra options and utilities for endpoints regeneration.
		endpoint.RegeneratorCell,

		// Redirect policy manages the Local Redirect Policies.
		redirectpolicy.Cell,

		// The node discovery cell provides the local node configuration and node discovery
		// which communicate changes in local node information to the API server or KVStore.
		nodediscovery.Cell,

		// Cgroup manager maintains Kubernetes and low-level metadata (cgroup path and
		// cgroup id) for local pods and their containers.
		cgroup.Cell,

		// NAT stats provides stat computation and tables for NAT map bpf maps.
		natStats.Cell,

		// Provide the logic to map DNS names matching Kubernetes services to the
		// corresponding ClusterIP, without depending on CoreDNS. Leveraged by etcd
		// and clustermesh.
		dial.ServiceResolverCell,

		// Provide resource groups to watch.
		cell.Provide(func() watchers.ResourceGroupFunc { return allResourceGroups }),

		// K8s Watcher provides the core k8s watchers
		watchers.Cell,

		// Provide pcap recorder
		recorder.Cell,

		// Provides a wrapper of the cilium config that can be watched dynamically
		dynamicconfig.Cell,

		// Provides the manager for WithDynamicFeature()
		// Which allows to group the cell lifecycles together and control the enablement
		// by leveraging the dynamicconfig.Cell.
		dynamiclifecycle.Cell,

		// Allows agent to monitor the configuration drift and publish drift metric
		driftchecker.Cell,

		// Runs the Hubble servers and Hubble metrics.
		hubble.Cell,

		// The feature Cell will retrieve information from all other cells /
		// configuration to describe, in form of prometheus metrics, which
		// features are enabled on the agent.
		features.Cell,

		// Determines priorities of data sources.
		source.Cell,

		// FQDN rules cell provides the FQDN proxy functionality.
		fqdn.Cell,

		// Cilium health infrastructure (host and endpoint connectivity)
		health.Cell,

		// Cilium Status Collector
		status.Cell,
	)
)

func configureAPIServer(cfg *option.DaemonConfig, s *server.Server, db *statedb.DB, swaggerSpec *server.Spec, logger *slog.Logger) {
	s.EnabledListeners = []string{"unix"}
	s.SocketPath = cfg.SocketPath
	s.ReadTimeout = apiTimeout
	s.WriteTimeout = apiTimeout

	const msg = "Required API option is disabled. This may prevent Cilium from operating correctly"
	hint := "Consider enabling this API in " + server.AdminEnableFlag
	for _, requiredAPI := range []string{
		"GetConfig",        // CNI: Used to detect detect IPAM mode
		"GetHealthz",       // Kubelet: daemon health checks
		"PutEndpointID",    // CNI: Provision the network for a new Pod
		"DeleteEndpointID", // CNI: Clean up networking for a deleted Pod
		"PostIPAM",         // CNI: Reserve IPs for new Pods
		"DeleteIPAMIP",     // CNI: Release IPs for deleted Pods
	} {
		if _, denied := swaggerSpec.DeniedAPIs[requiredAPI]; denied {
			logger.Warn(
				msg,
				logfields.Hint, hint,
				logfields.Params, requiredAPI,
			)
		}
	}
	api.DisableAPIs(logger, swaggerSpec.DeniedAPIs, s.GetAPI().AddMiddlewareFor)

	s.ConfigureAPI()

	// Add the /statedb HTTP handler
	mux := http.NewServeMux()
	mux.Handle("/", s.GetHandler())
	mux.Handle("/statedb/", http.StripPrefix("/statedb", db.HTTPHandler()))
	s.SetHandler(mux)
}

var pprofConfig = pprof.Config{
	Pprof:        false,
	PprofAddress: option.PprofAddressAgent,
	PprofPort:    option.PprofPortAgent,
}

// resourceGroups are all of the core Kubernetes and Cilium resource groups
// which the Cilium agent watches to implement CNI functionality.
func allResourceGroups(logger *slog.Logger, cfg watchers.WatcherConfiguration) (resourceGroups, waitForCachesOnly []string) {
	k8sGroups := []string{
		// To perform the service translation and have the BPF LB datapath
		// with the right service -> backend (k8s endpoints) translation.
		resources.K8sAPIGroupServiceV1Core,
		// Pods can contain labels which are essential for endpoints
		// being restored to have the right identity.
		resources.K8sAPIGroupPodV1Core,
		// To perform the service translation and have the BPF LB datapath
		// with the right service -> backend (k8s endpoints) translation.
		resources.K8sAPIGroupEndpointSliceOrEndpoint,
	}

	if option.NetworkPolicyEnabled(option.Config) {
		// Namespaces can contain labels which are essential for
		// endpoints being restored to have the right identity.
		// Namespaces are only used when network policies are enabled.
		k8sGroups = append(k8sGroups, resources.K8sAPIGroupNamespaceV1Core)
	}

	if cfg.K8sNetworkPolicyEnabled() {
		// When the flag is set,
		// We need all network policies in place before restoring to
		// make sure we are enforcing the correct policies for each
		// endpoint before restarting.
		waitForCachesOnly = append(waitForCachesOnly, resources.K8sAPIGroupNetworkingV1Core)
	}

	ciliumGroups, waitOnlyList := watchers.GetGroupsForCiliumResources(logger, k8sSynced.AgentCRDResourceNames())
	waitForCachesOnly = append(waitForCachesOnly, waitOnlyList...)

	return append(k8sGroups, ciliumGroups...), waitForCachesOnly
}
