//
//  Sensors.swift
//  TempHumidSensor
//
//  Created by STEVEN COUL on 9/9/20.
//  Copyright © 2020 STEVEN COUL. All rights reserved.
//

import Foundation

var single = Sensors()

class Sensors : NSObject, NetServiceBrowserDelegate, NetServiceDelegate {

    var browser = NetServiceBrowser()
    var service: NetService?
    
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
       print("Search about to begin")
     }

    func netServiceBrowserDidStopSearch(_ browser: NetServiceBrowser) {
      print("Search stopped")
    }

    func netServiceBrowser(_ browser: NetServiceBrowser, didFind svc: NetService, moreComing: Bool) {
      print("Discovered the service")
      print("- name:", svc.name)
      print("- type", svc.type)
      print("- domain:", svc.domain)

        service = svc
           service?.delegate = self
           service?.resolve(withTimeout: 5)
        
    }
    
    func netServiceDidResolveAddress(_ sender: NetService) {
      print("Resolved service")

      // Find the IPV4 address
      if let serviceIp = resolveIPv4(addresses: sender.addresses!) {
        print("Found IPV4:", serviceIp)
        print("port", sender.port )
      } else {
        print("Did not find IPV4 address")
      }

    }
    func run() {
        browser.delegate = self
        browser.searchForServices(ofType: "_temphumidsensor._tcp.", inDomain: "")
    }
    
    class func start() {
        single.run()
    }
}
