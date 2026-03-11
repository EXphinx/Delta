//
//  SwanGameInput.swift
//  SwanDeltaCore
//
//  Physical button mapping for WonderSwan.
//  Maps to absolute physical microswitches on the WS PCB.
//  rawValue = bit position in the 11-bit input bitmask.
//

import DeltaCore

@objc public enum SwanGameInput: Int, Input
{
    // X Cursor pad (right side on horizontal hold)
    case x1 = 0   // X Up
    case x2 = 1   // X Right
    case x3 = 2   // X Down
    case x4 = 3   // X Left
    
    // Y Cursor pad (left side on horizontal hold)
    case y1 = 4   // Y Up
    case y2 = 5   // Y Right
    case y3 = 6   // Y Down
    case y4 = 7   // Y Left
    
    // Action buttons
    case a     = 8
    case b     = 9
    case start = 10
    
    public var type: InputType {
        return .game(.ws)
    }
}
