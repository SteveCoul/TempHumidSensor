//
//  SensorScan.swift
//  TempHumidSensor
//
//  Created by STEVEN COUL on 9/10/20.
//  Copyright © 2020 STEVEN COUL. All rights reserved.
//

import Foundation

protocol SensorScanFound {
    func foundSensor( name: String, address: String )
}

class SensorScan : NSObject, NetServiceBrowserDelegate, NetServiceDelegate {
    
    var browser = NetServiceBrowser()
    var service_discovery = [NetService]()
    var callback : SensorScanFound? = nil
    
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
        service_discovery.append(svc)
    }
    
    func netServiceDidResolveAddress(_ sender: NetService) {
        if let serviceIp = resolveIPv4(addresses: sender.addresses!) {
            let str = String("http://" + serviceIp + ":" + String( sender.port ) )
            callback?.foundSensor(name: sender.name, address: str)
        }
    }

    func run( _ cb: SensorScanFound? = nil ) {
        callback = cb
        browser = NetServiceBrowser()
        service_discovery.removeAll()
        browser.delegate = self
        browser.searchForServices(ofType: "_temphumidsensor._tcp.", inDomain: "")
    }
}
