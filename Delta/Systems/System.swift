//
//  System.swift
//  Delta
//
//  Created by Riley Testut on 4/30/17.
//  Copyright © 2017 Riley Testut. All rights reserved.
//

import DeltaCore

import SNESDeltaCore
import GBADeltaCore
import GBCDeltaCore
import MelonDSDeltaCore
import SwanDeltaCore

enum System: CaseIterable
{
    case snes
    case gbc
    case gba
    case ds
    case ws
    
    static var registeredSystems: [System] {
        let systems = System.allCases.filter { Delta.registeredCores.keys.contains($0.gameType) }
        return systems
    }
    
    static var allCores: [DeltaCoreProtocol] {
        return [SNES.core, GBC.core, GBA.core, MelonDS.core, WonderSwan.core]
    }
}

extension System
{
    var localizedName: String {
        switch self
        {
        case .snes: return NSLocalizedString("Super Nintendo", comment: "")
        case .gbc: return NSLocalizedString("Game Boy Color", comment: "")
        case .gba: return NSLocalizedString("Game Boy Advance", comment: "")
        case .ds: return NSLocalizedString("Nintendo DS", comment: "")
        case .ws: return NSLocalizedString("WonderSwan", comment: "")
        }
    }
    
    var localizedShortName: String {
        switch self
        {
        case .snes: return NSLocalizedString("SNES", comment: "")
        case .gbc: return NSLocalizedString("GBC", comment: "")
        case .gba: return NSLocalizedString("GBA", comment: "")
        case .ds: return NSLocalizedString("DS", comment: "")
        case .ws: return NSLocalizedString("WS", comment: "")
        }
    }
    
    var localizedDisplayName: String {
        switch self
        {
        case .snes: return NSLocalizedString("Super Nintendo", comment: "")
        case .gbc: return NSLocalizedString("Game Boy Color", comment: "")
        case .gba: return NSLocalizedString("Game Boy Advance", comment: "")
        case .ds: return NSLocalizedString("Nintendo DS", comment: "")
        case .ws: return NSLocalizedString("WonderSwan", comment: "")
        }
    }
    
    var year: Int {
        switch self
        {
        case .snes: return 1990
        case .gbc: return 1998
        case .ws: return 1999
        case .gba: return 2001
        case .ds: return 2004
        }
    }
}

extension System
{
    var deltaCore: DeltaCoreProtocol {
        switch self
        {
        case .snes: return SNES.core
        case .gbc: return GBC.core
        case .gba: return GBA.core
        case .ds: return Settings.preferredCore(for: .ds) ?? MelonDS.core
        case .ws: return WonderSwan.core
        }
    }
    
    var gameType: DeltaCore.GameType {
        switch self
        {
        case .snes: return .snes
        case .gbc: return .gbc
        case .gba: return .gba
        case .ds: return .ds
        case .ws: return .ws
        }
    }
    
    init?(gameType: DeltaCore.GameType)
    {
        switch gameType
        {
        case GameType.snes: self = .snes
        case GameType.gbc: self = .gbc
        case GameType.gba: self = .gba
        case GameType.ds: self = .ds
        case GameType.ws: self = .ws
        default: return nil
        }
    }
}

extension DeltaCore.GameType
{
    init?(fileExtension: String)
    {
        switch fileExtension.lowercased()
        {
        case "smc", "sfc", "fig": self = .snes
        case "gbc", "gb": self = .gbc
        case "gba": self = .gba
        case "ds", "nds": self = .ds
        case "ws", "wsc": self = .ws
        default: return nil
        }
    }
}
