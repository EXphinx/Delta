//
//  WonderSwan.swift
//  SwanDeltaCore
//
//  DeltaCoreProtocol implementation for WonderSwan / WonderSwan Color.
//  Modeled after GBA.swift from GBADeltaCore.
//

import Foundation
import AVFoundation
import DeltaCore

public extension GameType
{
    static let ws = GameType("com.delta.wonderswan")
}

public struct WonderSwan: DeltaCoreProtocol
{
    public static let core = WonderSwan()
    
    public var name: String { "SwanDeltaCore" }
    public var identifier: String { "com.delta.wonderswan" }
    
    public var gameType: GameType { .ws }
    public var gameInputType: Input.Type { SwanGameInput.self }
    public var gameSaveFileExtension: String { "sav" }
    
    // WonderSwan audio: 48kHz stereo interleaved Int16
    public let audioFormat = AVAudioFormat(
        commonFormat: .pcmFormatInt16,
        sampleRate: 48000,
        channels: 2,
        interleaved: true
    )!
    
    // Video: 224×144 native resolution, BGRA8 pixel format
    // (Mednafen outputs XRGB8888 which maps to BGRA8 on little-endian)
    public let videoFormat = VideoFormat(
        format: .bitmap(.bgra8),
        dimensions: CGSize(width: 224, height: 144)
    )
    
    // No cheat support in v1
    public var supportedCheatFormats: Set<CheatFormat> {
        return []
    }
    
    public let emulatorBridge: EmulatorBridging = SwanEmulatorBridge.shared
    
    private init() {}
}

// Expose DeltaCore properties to Objective-C
public extension SwanEmulatorBridge
{
    @objc(swanResources) class var __swanResources: Bundle {
        return WonderSwan.core.resourceBundle
    }
    
    @objc(coreDirectoryURL) class var __coreDirectoryURL: URL {
        return _coreDirectoryURL
    }
}

private let _coreDirectoryURL = WonderSwan.core.directoryURL
