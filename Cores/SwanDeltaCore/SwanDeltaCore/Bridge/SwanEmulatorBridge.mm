//
//  SwanEmulatorBridge.mm
//  SwanDeltaCore
//
//  Objective-C++ bridge implementing DLTAEmulatorBridging protocol.
//  Modeled after GBAEmulatorBridge.mm.
//

#import "SwanEmulatorBridge.h"
#import <DeltaCore/DeltaCore-Swift.h>

#include "../Engine/swan_core.h"

#include <string.h>

#define WS_WIDTH  224
#define WS_HEIGHT 144

@interface SwanEmulatorBridge () <DLTAEmulatorBridging>

@property (nonatomic, copy, nullable, readwrite) NSURL *gameURL;
@property (nonatomic, assign) uint32_t activatedInputs;
@property (nonatomic, assign, readwrite) BOOL isColorMode;

@end

@implementation SwanEmulatorBridge
@synthesize audioRenderer = _audioRenderer;
@synthesize videoRenderer = _videoRenderer;
@synthesize saveUpdateHandler = _saveUpdateHandler;

+ (instancetype)sharedBridge
{
    static SwanEmulatorBridge *_bridge = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _bridge = [[self alloc] init];
    });
    return _bridge;
}

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        _activatedInputs = 0;
        _isColorMode = NO;
    }
    return self;
}

#pragma mark - Emulation State -

- (void)startWithGameURL:(NSURL *)URL
{
    self.gameURL = URL;
    
    // Initialize engine
    swan_init(48000.0);
    
    // Load ROM from memory (zero file-I/O in engine)
    NSData *romData = [NSData dataWithContentsOfURL:URL];
    if (!romData || romData.length == 0) {
        NSLog(@"SwanDeltaCore: Failed to load ROM data from %@", URL);
        return;
    }
    
    if (!swan_load_rom((const uint8_t *)romData.bytes, romData.length)) {
        NSLog(@"SwanDeltaCore: swan_load_rom failed for %@", URL);
        return;
    }
    
    // Probe hardware mode (for future rendering pipeline)
    self.isColorMode = swan_is_color_mode();
    
    NSLog(@"SwanDeltaCore: Loaded ROM (%lu bytes), Color=%@", 
          (unsigned long)romData.length, self.isColorMode ? @"YES" : @"NO");
}

- (void)stop
{
    swan_shutdown();
}

- (void)pause
{
    // No special handling needed
}

- (void)resume
{
    // No special handling needed
}

#pragma mark - Game Loop -

- (void)runFrameAndProcessVideo:(BOOL)processVideo
{
    // Build input bitmask from activated inputs
    uint16_t inputMask = (uint16_t)(self.activatedInputs & 0x7FF);
    
    // Run one frame of emulation
    int audioFrameCount = swan_run_frame(inputMask);
    
    // Copy video data to DeltaCore's video renderer
    if (processVideo)
    {
        const void *videoData = swan_get_video_buffer();
        if (videoData && self.videoRenderer.videoBuffer)
        {
            int pixBytes = swan_get_pixel_bytes();
            
            // Copy per-row to handle potential stride differences
            for (int y = 0; y < WS_HEIGHT; y++)
            {
                memcpy((uint8_t *)self.videoRenderer.videoBuffer + y * WS_WIDTH * pixBytes,
                       (const uint8_t *)videoData + y * WS_WIDTH * pixBytes,
                       WS_WIDTH * pixBytes);
            }
            
            [self.videoRenderer processFrame];
        }
    }
    
    // Write audio data to DeltaCore's audio renderer
    if (audioFrameCount > 0)
    {
        const int16_t *audioData = swan_get_audio_buffer();
        if (audioData)
        {
            // Each audio frame = 2 channels × 2 bytes = 4 bytes
            [self.audioRenderer.audioBuffer writeBuffer:(uint8_t *)audioData 
                                                  size:audioFrameCount * 4];
        }
    }
}

#pragma mark - Inputs -

- (void)activateInput:(NSInteger)input value:(double)value playerIndex:(NSInteger)playerIndex
{
    self.activatedInputs |= (1 << (uint32_t)input);
}

- (void)deactivateInput:(NSInteger)input playerIndex:(NSInteger)playerIndex
{
    self.activatedInputs &= ~(1 << (uint32_t)input);
}

- (void)resetInputs
{
    self.activatedInputs = 0;
}

#pragma mark - Save States -

- (void)saveSaveStateToURL:(NSURL *)URL
{
    size_t stateSize = swan_get_state_size();
    if (stateSize == 0) return;
    
    NSMutableData *stateData = [NSMutableData dataWithLength:stateSize];
    if (swan_save_state((uint8_t *)stateData.mutableBytes, stateSize))
    {
        [stateData writeToURL:URL atomically:YES];
    }
}

- (void)loadSaveStateFromURL:(NSURL *)URL
{
    NSData *stateData = [NSData dataWithContentsOfURL:URL];
    if (stateData)
    {
        swan_load_state((const uint8_t *)stateData.bytes, stateData.length);
    }
}

#pragma mark - Game Saves (SRAM/EEPROM) -

- (void)saveGameSaveToURL:(NSURL *)URL
{
    size_t saveSize = swan_get_save_size();
    if (saveSize == 0) return;
    
    NSMutableData *saveData = [NSMutableData dataWithLength:saveSize];
    if (swan_read_save((uint8_t *)saveData.mutableBytes, saveSize))
    {
        [saveData writeToURL:URL atomically:YES];
    }
}

- (void)loadGameSaveFromURL:(NSURL *)URL
{
    NSData *saveData = [NSData dataWithContentsOfURL:URL];
    if (saveData && saveData.length > 0)
    {
        swan_write_save((const uint8_t *)saveData.bytes, saveData.length);
    }
}

#pragma mark - Cheats -

- (BOOL)addCheatCode:(NSString *)cheatCode type:(NSString *)type
{
    // Cheat support not implemented in v1
    return NO;
}

- (void)resetCheats
{
    // No-op
}

- (void)updateCheats
{
    // No-op
}

#pragma mark - Getters/Setters -

- (NSTimeInterval)frameDuration
{
    // WonderSwan native refresh rate: 3072000 / (159 * 256) ≈ 75.47 Hz
    return 1.0 / (3072000.0 / (159.0 * 256.0));
}

@end
