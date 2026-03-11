//
//  SwanEmulatorBridge.h
//  SwanDeltaCore
//
//  Bridge layer between C++ engine and DeltaCore framework.
//

#import <Foundation/Foundation.h>

@protocol DLTAEmulatorBridging;

NS_ASSUME_NONNULL_BEGIN

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
@interface SwanEmulatorBridge : NSObject <DLTAEmulatorBridging>
#pragma clang diagnostic pop

@property (class, nonatomic, readonly) SwanEmulatorBridge *sharedBridge;

/// YES if the loaded ROM is WonderSwan Color, NO for original WS (mono).
/// Pre-reserved for future rendering pipeline (color palette / ghosting).
@property (nonatomic, assign, readonly) BOOL isColorMode;

@end

NS_ASSUME_NONNULL_END
