//
//  SensorList.swift
//  TempHumidSensor
//
//  Created by STEVEN COUL on 9/10/20.
//  Copyright © 2020 STEVEN COUL. All rights reserved.
//

import Foundation

func now() -> Double {
    return NSDate().timeIntervalSince1970
}


class Sensor {
    var last_seen = 0.0
    var name = ""
    var address = ""
    
    init( name: String, address: String ) {
        self.name = name;
        self.address = address;
        self.last_seen = now()
    }
}

class SensorList {
 
    var list = [Sensor]()
    
    func find( name: String, address: String ) -> Sensor? {
        var rc : Sensor? = nil
        list.forEach {
            if (( $0.name == name ) && ( $0.address == address ) ) {
                rc = $0
            }
        }
        return rc
    }
    
    func found(name: String, address: String) {
        let s = find( name: name, address: address )
        if ( s == nil ) {
            print("Adding", name, "at", address )
            list.append( Sensor( name: name, address: address ) )
        } else {
            print("Keep alive", name, "at", address )
            s!.last_seen = now()
        }
        
        list.sort { $0.name < $1.name }
    }

    // Expire any device we haven't seen in (age) seconds
    func expire( _ age: Float ) {
        var new = [Sensor]()
        while ( list.isEmpty == false ) {
            let o = list.popLast()
            if ( o != nil ) {
                if ( o!.last_seen + Double( age ) < now() ) {
                    print("Expiring", o!.name, "at", o!.address )
                } else {
                    new.append( o! )
                }
            }
        }
        new.sort { $0.name < $1.name }
        list = new
    }
}
