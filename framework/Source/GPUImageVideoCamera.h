#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import "GPUImageContext.h"
#import "GPUImageOutput.h"
#import "GPUImageColorConversion.h"

//Optionally override the YUV to RGB matrices
void setColorConversion601( GLfloat conversionMatrix[9] );
void setColorConversion601FullRange( GLfloat conversionMatrix[9] );
void setColorConversion709( GLfloat conversionMatrix[9] );


//Delegate Protocal for Face Detection.
// kCVPixelFormatType_32BGRA
// kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
// kCVPixelFormatType_420YpCbCr8BiPlanarFullRange

/*
 yuv格式是一种图片储存格式，跟RGB格式类似。yuv中，y表示亮度，单独只有y数据就可以形成一张图片，只不过这张图片是灰色的。u和v表示色差(u和v也被称为：Cb－蓝色差，Cr－红色差)。最早的电视信号，为了兼容黑白电视，采用的就是yuv格式。一张yuv的图像，去掉uv，只保留y，这张图片就是黑白的。yuv可以通过抛弃色差来进行带宽优化。比如yuv420格式图像相比RGB来说，要节省一半的字节大小，抛弃相邻的色差对于人眼来说，差别不大。
 
 yuv图像占用字节数为 ：
 size = width * height + (width * height) / 4 + (width * height) / 4
 RGB格式的图像占用字节数为:
 size = width * height * 3
 RGBA格式的图像占用字节数为:
 size = width * height * 4
 yuv420也包含不同的数据排列格式：I420，NV12，NV21.
 I420格式：y,u,v 3个部分分别存储：Y0,Y1…Yn,U0,U1…Un/2,V0,V1…Vn/2
 NV12格式：y和uv 2个部分分别存储：Y0,Y1…Yn,U0,V0,U1,V1…Un/2,Vn/2
 NV21格式：同NV12，只是U和V的顺序相反。
 
 iOS相机输出图片格式，下图为设备支持的格式：
 设备支持格式
 kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange = '420v'，表示输出的视频格式为NV12；范围： (luma=[16,235] chroma=[16,240])
 kCVPixelFormatType_420YpCbCr8BiPlanarFullRange = '420f'，表示输出的视频格式为NV12；范围： (luma=[0,255] chroma=[1,255])
 kCVPixelFormatType_32BGRA = 'BGRA', 输出的是BGRA的格式
 */
@protocol GPUImageVideoCameraDelegate <NSObject>

@optional
- (void)willOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer;
@end


/**
 A GPUImageOutput that provides frames from either camera
*/
@interface GPUImageVideoCamera : GPUImageOutput <AVCaptureVideoDataOutputSampleBufferDelegate, AVCaptureAudioDataOutputSampleBufferDelegate>
{
    NSUInteger numberOfFramesCaptured;
    CGFloat totalFrameTimeDuringCapture;
    
    AVCaptureSession *_captureSession;
    AVCaptureDevice *_inputCamera;
    AVCaptureDevice *_microphone;
    AVCaptureDeviceInput *videoInput;
	AVCaptureVideoDataOutput *videoOutput;

    BOOL capturePaused;
    GPUImageRotationMode outputRotation, internalRotation;
    dispatch_semaphore_t frameRenderingSemaphore;
        
    BOOL captureAsYUV;
    GLuint luminanceTexture, chrominanceTexture;

    __unsafe_unretained id<GPUImageVideoCameraDelegate> _delegate;
}

/// Whether or not the underlying AVCaptureSession is running
@property(readonly, nonatomic) BOOL isRunning;

/// The AVCaptureSession used to capture from the camera
@property(readonly, retain, nonatomic) AVCaptureSession *captureSession;

/// This enables the capture session preset to be changed on the fly
@property (readwrite, nonatomic, copy) NSString *captureSessionPreset;

/// This sets the frame rate of the camera (iOS 5 and above only)
/**
 Setting this to 0 or below will set the frame rate back to the default setting for a particular preset.
 */
@property (readwrite) int32_t frameRate;

/// Easy way to tell which cameras are present on device
@property (readonly, getter = isFrontFacingCameraPresent) BOOL frontFacingCameraPresent;
@property (readonly, getter = isBackFacingCameraPresent) BOOL backFacingCameraPresent;

/// This enables the benchmarking mode, which logs out instantaneous and average frame times to the console
// 实时日志输出
@property(readwrite, nonatomic) BOOL runBenchmark;

/// Use this property to manage camera settings. Focus point, exposure point, etc.
// 正在使用的相机对象，方便设置参数
@property(readonly) AVCaptureDevice *inputCamera;

/// This determines the rotation applied to the output image, based on the source material
// 输出图片的方向
@property(readwrite, nonatomic) UIInterfaceOrientation outputImageOrientation;

/// These properties determine whether or not the two camera orientations should be mirrored. By default, both are NO.
// 前置相机水平镜像
@property(readwrite, nonatomic) BOOL horizontallyMirrorFrontFacingCamera, horizontallyMirrorRearFacingCamera;

@property(nonatomic, assign) id<GPUImageVideoCameraDelegate> delegate;

/// @name Initialization and teardown

/** Begin a capture session
 
 See AVCaptureSession for acceptable values
 
 @param sessionPreset Session preset to use
 @param cameraPosition Camera to capture from
 */
- (void)printSupportedPixelFormats;
- (id)initWithSessionPreset:(NSString *)sessionPreset cameraPosition:(AVCaptureDevicePosition)cameraPosition;

/** Add audio capture to the session. Adding inputs and outputs freezes the capture session momentarily, so you
    can use this method to add the audio inputs and outputs early, if you're going to set the audioEncodingTarget 
    later. Returns YES is the audio inputs and outputs were added, or NO if they had already been added.
 */
// 添加、移除音频输入输出
- (BOOL)addAudioInputsAndOutputs;

/** Remove the audio capture inputs and outputs from this session. Returns YES if the audio inputs and outputs
    were removed, or NO is they hadn't already been added.
 */
- (BOOL)removeAudioInputsAndOutputs;

/** Tear down the capture session
 */
// 移除所有输入输出
- (void)removeInputsAndOutputs;

/// @name Manage the camera video stream
// 开始、关闭、暂停、恢复相机捕获
/** Start camera capturing
 */
- (void)startCameraCapture;

/** Stop camera capturing
 */
- (void)stopCameraCapture;

/** Pause camera capturing
 */
- (void)pauseCameraCapture;

/** Resume camera capturing
 */
- (void)resumeCameraCapture;

// 处理音视频
/** Process a video sample
 @param sampleBuffer Buffer to process
 */
- (void)processVideoSampleBuffer:(CMSampleBufferRef)sampleBuffer;

/** Process an audio sample
 @param sampleBuffer Buffer to process
 */
- (void)processAudioSampleBuffer:(CMSampleBufferRef)sampleBuffer;

// 获取相机相关参数
/** Get the position (front, rear) of the source camera
 */
- (AVCaptureDevicePosition)cameraPosition;

/** Get the AVCaptureConnection of the source camera
 */
- (AVCaptureConnection *)videoCaptureConnection;

// 变换相机
/** This flips between the front and rear cameras
 */
- (void)rotateCamera;

/// @name Benchmarking

// 获取平均帧率
/** When benchmarking is enabled, this will keep a running average of the time from uploading, processing, and final recording or display
 */
- (CGFloat)averageFrameDurationDuringCapture;

// 重置相关基准
- (void)resetBenchmarkAverage;

+ (BOOL)isBackFacingCameraPresent;
+ (BOOL)isFrontFacingCameraPresent;

@end
