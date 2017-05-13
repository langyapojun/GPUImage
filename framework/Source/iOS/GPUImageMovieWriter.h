#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "GPUImageContext.h"

/*
 AVAssetWriter支持的音视频格式比较多，具体可以参考下面的表格：
 
 定义                         扩展名
 AVFileTypeQuickTimeMovie	.mov 或 .qt
 AVFileTypeMPEG4            .mp4
 AVFileTypeAppleM4V         .m4v
 AVFileTypeAppleM4A         .m4a
 AVFileType3GPP             .3gp 或 .3gpp 或 .sdv
 AVFileType3GPP2            .3g2 或 .3gp2
 AVFileTypeCoreAudioFormat	.caf
 AVFileTypeWAVE             .wav 或 .wave 或 .bwf
 AVFileTypeAIFF             .aif 或 .aiff
 AVFileTypeAIFC             .aifc 或 .cdda
 AVFileTypeAMR              .amr
 AVFileTypeWAVE             .wav 或 .wave 或 .bwf
 AVFileTypeMPEGLayer3       .mp3
 AVFileTypeSunAU            .au 或 .snd
 AVFileTypeAC3              .ac3
 AVFileTypeEnhancedAC3      .eac3
 */

extern NSString *const kGPUImageColorSwizzlingFragmentShaderString;

@protocol GPUImageMovieWriterDelegate <NSObject>

@optional
- (void)movieRecordingCompleted;
- (void)movieRecordingFailedWithError:(NSError*)error;

@end

@interface GPUImageMovieWriter : NSObject <GPUImageInput>
{
    BOOL alreadyFinishedRecording;
    
    NSURL *movieURL;
    NSString *fileType;
	AVAssetWriter *assetWriter;
	AVAssetWriterInput *assetWriterAudioInput;
	AVAssetWriterInput *assetWriterVideoInput;
    AVAssetWriterInputPixelBufferAdaptor *assetWriterPixelBufferInput;
    
    GPUImageContext *_movieWriterContext;
    CVPixelBufferRef renderTarget;
    CVOpenGLESTextureRef renderTexture;

    CGSize videoSize;
    GPUImageRotationMode inputRotation;
}
// 是否有音频
@property(readwrite, nonatomic) BOOL hasAudioTrack;
// 是否不处理音频
@property(readwrite, nonatomic) BOOL shouldPassthroughAudio;
// 标记不被再次使用
@property(readwrite, nonatomic) BOOL shouldInvalidateAudioSampleWhenDone;
// 完成与失败回调
@property(nonatomic, copy) void(^completionBlock)(void);
@property(nonatomic, copy) void(^failureBlock)(NSError*);
@property(nonatomic, assign) id<GPUImageMovieWriterDelegate> delegate;
// 是否实时编码视频
@property(readwrite, nonatomic) BOOL encodingLiveVideo;
// 音视频就绪回调
@property(nonatomic, copy) BOOL(^videoInputReadyCallback)(void);
@property(nonatomic, copy) BOOL(^audioInputReadyCallback)(void);
// 处理音频回调
@property(nonatomic, copy) void(^audioProcessingCallback)(SInt16 **samplesRef, CMItemCount numSamplesInBuffer);
@property(nonatomic) BOOL enabled;
// 获取AVAssetWriter
@property(nonatomic, readonly) AVAssetWriter *assetWriter;
// 获取开始到前一帧的时长
@property(nonatomic, readonly) CMTime duration;
@property(nonatomic, assign) CGAffineTransform transform;
@property(nonatomic, copy) NSArray *metaData;
@property(nonatomic, assign, getter = isPaused) BOOL paused;
@property(nonatomic, retain) GPUImageContext *movieWriterContext;

// Initialization and teardown
- (id)initWithMovieURL:(NSURL *)newMovieURL size:(CGSize)newSize;
- (id)initWithMovieURL:(NSURL *)newMovieURL size:(CGSize)newSize fileType:(NSString *)newFileType outputSettings:(NSDictionary *)outputSettings;

// 设置需要写入音频数据
- (void)setHasAudioTrack:(BOOL)hasAudioTrack audioSettings:(NSDictionary *)audioOutputSettings;

// Movie recording
// 开始、结束、取消录制
- (void)startRecording;
- (void)startRecordingInOrientation:(CGAffineTransform)orientationTransform;
- (void)finishRecording;
- (void)finishRecordingWithCompletionHandler:(void (^)(void))handler;
- (void)cancelRecording;

// 处理音频
- (void)processAudioBuffer:(CMSampleBufferRef)audioBuffer;
// 处理同步videoInputReadyCallback、audioInputReadyCallback回调
- (void)enableSynchronizationCallbacks;

@end
