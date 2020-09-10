//
//  ViewController.swift
//  TempHumidSensor
//
//  Created by STEVEN COUL on 9/10/20.
//  Copyright © 2020 STEVEN COUL. All rights reserved.
//

import UIKit


class ViewController: UIViewController, SensorScanFound {

    var scanner = SensorScan()
    var list = SensorList()
    
    func foundSensor(name: String, address: String) {
        list.found( name: name, address: address )
    }
    
    func update() -> Void {
        list.expire(20.0)
        scanner.run(self)
        DispatchQueue.main.asyncAfter(deadline: DispatchTime.now() + 10, execute: { self.update() })
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        update();
    }


}

