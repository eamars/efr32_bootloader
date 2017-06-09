// File: rename-ezsp-functions.h
//
// Description: The file names certain functions that run on host to have 
// the ezsp- prefix for functions instead of the ember- prefix.
//
// Copyright 2014 by Silicon Labs. Al rights reserved.

//Utility Frames
#define emberGetLibraryStatus			  			  ezspGetLibraryStatus

//Networking Frames
#define emberSetManufacturerCode 		  			  ezspSetManufacturerCode
#define emberSetPowerDescriptor  		  			  ezspSetPowerDescriptor
#define emberNetworkInit 				  			  ezspNetworkInit
#define emberNetworkInitExtended 		  			  ezspNetworkInitExtended
#define emberNetworkState				  			  ezspNetworkState
#define emberFormNetwork		 	      			  ezspFormNetwork
#define emberJoinNetwork		 	      			  ezspJoinNetwork
#define emberNetworkState				  			  ezspNetworkState
#define emberStartScan					  			  ezspStartScan
#define emberStopScan					  			  ezspStopScan
#define emberFormNetwork				  			  ezspFormNetwork
#define emberJoinNetwork				  			  ezspJoinNetwork
#define emberLeaveNetwork				  			  ezspLeaveNetwork
#define emberPermitJoining				  			  ezspPermitJoining
#define emberEnergyScanRequest	 	      			  ezspEnergyScanRequest
#define emberGetNodeId			 	      			  ezspGetNodeId
#define emberGetNeighbor		 	      			  ezspGetNeighbor
#define emberNeighborCount				  			  ezspNeighborCount	
#define emberGetRouteTableEntry  	      			  ezspGetRouteTableEntry
#define emberSetRadioPower		 		  			  ezspSetRadioPower
#define emberSetRadioChannel			  			  ezspSetRadioChannel

//Binding Frames
#define emberClearBindingTable			  			  ezspClearBindingTable
#define emberSetBinding					  			  ezspSetBinding
#define emberGetBinding			 		  			  ezspGetBinding
#define emberDeleteBinding				  			  ezspDeleteBinding
#define emberBindingIsActive			  			  ezspBindingIsActive
#define emberGetBindingRemoteNodeId		  			  ezspGetBindingRemoteNodeId
#define emberSetBindingRemoteNodeId		  			  ezspSetBindingRemoteNodeId



//Messaging Frames
#define emberSendManyToOneRouteRequest    			  ezspSendManyToOneRouteRequest
#define emberAddressTableEntryIsActive	  			  ezspAddressTableEntryIsActive
#define emberSetAddressTableRemoteEui64	  			  ezspSetAddressTableRemoteEui64
#define emberSetAddressTableRemoteNodeId  			  ezspSetAddressTableRemoteNodeId
#define emberGetAddressTableRemoteEui64	 			  ezspGetAddressTableRemoteEui64
#define emberGetAddressTableRemoteNodeId  			  ezspGetAddressTableRemoteNodeId
#define emberSetExtendedTimeout			  			  ezspSetExtendedTimeout
#define emberGetExtendedTimeout			  			  ezspGetExtendedTimeout
#define emberLookupNodeIdByEui64		  			  ezspLookupNodeIdByEui64
#define emberLookupEui64ByNodeId		  			  ezspLookupEui64ByNodeId

//Security Frames
#define emberSetInitialSecurityState	  			  ezspSetInitialSecurityState
#define emberGetCurrentSecurityState	  			  ezspGetCurrentSecurityState
#define emberGetKey						  			  ezspGetKey
#define emberGetKeyTableEntry			  			  ezspGetKeyTableEntry
#define emberSetKeyTableEntry			  			  ezspSetKeyTableEntry
#define emberFindKeyTableEntry			  			  ezspFindKeyTableEntry
#define emberAddOrUpdateKeyTableEntry	  			  ezspAddOrUpdateKeyTableEntry
#define emberEraseKeyTableEntry			  			  ezspEraseKeyTableEntry
#define emberClearKeyTable				  			  ezspClearKeyTable
#define emberRequestLinkKey				  			  ezspRequestLinkKey

//ezsprustCenter Frames
#define emberBroadcastNextNetworkKey 	  			  ezspBroadcastNextNetworkKey
#define emberBroadcastNetworkKeySwitch	  			  ezspBroadcastNetworkKeySwitch
#define emberBecomeTrustCenter			  			  ezspBecomeTrustCenter

// Certificate Based Key Exchange (CBKE)
#define emberGenerateCbkeKeys				  		  ezspGenerateCbkeKeys
#define emberGenerateCbkeKeys283k1			  		  ezspGenerateCbkeKeys283k1
#define emberCalculateSmacs				  			  ezspCalculateSmacs
#define emberCalculateSmacs283k1		  			  ezspCalculateSmacs283k1
#define emberGetCertificate				  			  ezspGetCertificate
#define emberGetCertificate283k1		  			  ezspGetCertificate283k1
#define emberDsaVerify					  			  ezspDsaVerify
#define emberSetPreinstalledCbkeData	  			  ezspSetPreinstalledCbkeData
#define emberSetPreinstalledCbkeData283k1 			  ezspSetPreinstalledCbkeData283k1
#define emberClearTemporaryDataMaybeStoreLinkKey	  ezspClearTemporaryDataMaybeStoreLinkKey
#define emberClearTemporaryDataMaybeStoreLinkKey283k1 ezspClearTemporaryDataMaybeStoreLinkKey283k1

#define emberDGpSend ezspDGpSend
#define emberGpProxyTableProcessGpPairing ezspGpProxyTableProcessGpPairing
