#import "GLProgram.h"
#import "GPUImageFramebuffer.h"
#import "GPUImageFramebufferCache.h"

//GPUImageContext类，提供OpenGL ES基本上下文，GPUImage相关处理线程，GLProgram缓存、帧缓存。由于是上下文对象，因此该模块提供的更多是存取、设置相关的方法
//GPUImageContext 是GPUImage的上下文对象，管理着OpenGLES上下文对象，管理着GL程序，管理着帧缓存等GPUImage中的基本组件。

#define GPUImageRotationSwapsWidthAndHeight(rotation) ((rotation) == kGPUImageRotateLeft || (rotation) == kGPUImageRotateRight || (rotation) == kGPUImageRotateRightFlipVertical || (rotation) == kGPUImageRotateRightFlipHorizontal)

typedef NS_ENUM(NSUInteger, GPUImageRotationMode) {
	kGPUImageNoRotation,
	kGPUImageRotateLeft,
	kGPUImageRotateRight,
	kGPUImageFlipVertical,
	kGPUImageFlipHorizonal,
	kGPUImageRotateRightFlipVertical,
	kGPUImageRotateRightFlipHorizontal,
	kGPUImageRotate180
};

@interface GPUImageContext : NSObject

// GPUImage处理OpenGL回执的相关队列：串行队列
@property(readonly, nonatomic) dispatch_queue_t contextQueue;
// 当前使用的着色器程序
@property(readwrite, retain, nonatomic) GLProgram *currentShaderProgram;
// OpenGLES上下文对象
@property(readonly, retain, nonatomic) EAGLContext *context;
// CoreVideo中的纹理缓存
@property(readonly) CVOpenGLESTextureCacheRef coreVideoTextureCache;
// 帧缓存
@property(readonly) GPUImageFramebufferCache *framebufferCache;

// 获取队列标识
+ (void *)contextKey;
// 单例对象
+ (GPUImageContext *)sharedImageProcessingContext;
// 获取处理队列
+ (dispatch_queue_t)sharedContextQueue;
// 帧缓存
+ (GPUImageFramebufferCache *)sharedFramebufferCache;
// 设置当前上下文
+ (void)useImageProcessingContext;
- (void)useAsCurrentContext;
// 设置当前的GL程序
+ (void)setActiveShaderProgram:(GLProgram *)shaderProgram;
- (void)setContextShaderProgram:(GLProgram *)shaderProgram;
// 获取设备OpenGLES相关特性的支持情况
+ (GLint)maximumTextureSizeForThisDevice;
+ (GLint)maximumTextureUnitsForThisDevice;
+ (GLint)maximumVaryingVectorsForThisDevice;
+ (BOOL)deviceSupportsOpenGLESExtension:(NSString *)extension;
+ (BOOL)deviceSupportsRedTextures;
+ (BOOL)deviceSupportsFramebufferReads;
// 纹理大小调整，保证纹理不超过OpenGLES支持最大的尺寸
+ (CGSize)sizeThatFitsWithinATextureForSize:(CGSize)inputSize;

// 将渲染缓存呈现在设备上
- (void)presentBufferForDisplay;
// 创建GLProgram，首先在缓存中查找，如果没有则创建
- (GLProgram *)programForVertexShaderString:(NSString *)vertexShaderString fragmentShaderString:(NSString *)fragmentShaderString;

// 创建Sharegroup
- (void)useSharegroup:(EAGLSharegroup *)sharegroup;

// Manage fast texture upload
// 判断是否支持CoreVideo的快速纹理上传
+ (BOOL)supportsFastTextureUpload;

@end

// GPUImageInput 是GPUImage中的一个重要的协议，实现这个协议的类表示这个类能接受帧缓存的输入，在响应链中每一个中间节点都能够接受输入经过它的处理之后又能输出给下一个节点。正式这样的过程构成了一个响应链条，这也是叠加滤镜、组合滤镜的基础。
// GPUImage中实现GPUImageInput的协议的类比较多，常见的有 GPUImageFilter、GPUImageView、GPUImageRawDataOutput、GPUImageMovieWriter 等.
@protocol GPUImageInput <NSObject>
// 准备下一个要使用的帧
- (void)newFrameReadyAtTime:(CMTime)frameTime atIndex:(NSInteger)textureIndex;
// 设置输入的帧缓冲对象以及纹理索引
- (void)setInputFramebuffer:(GPUImageFramebuffer *)newInputFramebuffer atIndex:(NSInteger)textureIndex;
// 下一个有效的纹理索引
- (NSInteger)nextAvailableTextureIndex;
// 设置目标的尺寸
- (void)setInputSize:(CGSize)newSize atIndex:(NSInteger)textureIndex;
// 设置旋转方式
- (void)setInputRotation:(GPUImageRotationMode)newInputRotation atIndex:(NSInteger)textureIndex;
// 输出缓冲区的最大尺寸
- (CGSize)maximumOutputSize;
// 输入处理结束
- (void)endProcessing;
// 是否忽略渲染目标的更新
- (BOOL)shouldIgnoreUpdatesToThisTarget;
// 是否启动渲染目标
- (BOOL)enabled;
// 是否为单色输入
- (BOOL)wantsMonochromeInput;
// 设置单色输入
- (void)setCurrentlyReceivingMonochromeInput:(BOOL)newValue;
@end
