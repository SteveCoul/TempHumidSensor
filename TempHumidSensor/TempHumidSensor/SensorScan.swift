//
//  SensorScan.swift
//  TempHumidSensor
//
//  Created by STEVEN COUL on 9/10/20.
//  Copyright © 2020 STEVEN COUL. All rights reserved.
//

import Foundation

class SensorScan : NSObject, NetServiceBrowserDelegate, NetServiceDelegate {
    
    var browser = NetServiceBrowser()
    var service_discovery = [NetService]()
    
    func resolveIPv4(addresses: [Data]) -> String? {
        var result: String?
        for addr in addresses {
            let data = addr as NSData
            var storage = sockaddr_storage()
            data.getBytes(&storage, length: MemoryLayout<sockaddr_storage>.size)
            if Int32(storage.ss_family) == AF_INET {
                let addr4 = withUnsafePointer(to: &storage) {
                    $0.withMemoryRebound(to: sockaddr_in.self, capacity: 1) {
                        $0.pointee
                    }
                }
                if let ip = String(cString: inet_ntoa(addr4.sin_addr), encoding: .ascii) {
                    result = ip
                    break
                }
            }
        }
        return result
    }
    
    func netServiceBrowserWillSearch(_ browser: NetServiceBrowser) {
    }
    
    func netServiceBrowserDidStopSearch(_ browser: NetServiceBrowser) {
    }

    func netServiceBrowser(_ browser: NetServiceBrowser, didFind svc: NetService, moreComing: Bool) {
        svc.delegate = self
        svc.resolve(withTimeout: 5)
        print( svc, moreComing )
        service_discovery.append(svc)
    }
    
    func netServiceDidResolveAddress(_ sender: NetService) {
        if let serviceIp = resolveIPv4(addresses: sender.addresses!) {
            var str = String("http://" + serviceIp + ":" + String( sender.port ) )
        }
    }

    func run() {
        browser = NetServiceBrowser()
        browser.delegate = self
        browser.searchForServices(ofType: "_temphumidsensor._tcp.", inDomain: "")
    }
}