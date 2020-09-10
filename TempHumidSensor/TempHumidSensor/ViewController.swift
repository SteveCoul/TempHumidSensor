//
//  ViewController.swift
//  TempHumidSensor
//
//  Created by STEVEN COUL on 9/10/20.
//  Copyright © 2020 STEVEN COUL. All rights reserved.
//

import UIKit

class ViewController: UIViewController {

    var scanner = SensorScan()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        scanner.run()
    }


}

