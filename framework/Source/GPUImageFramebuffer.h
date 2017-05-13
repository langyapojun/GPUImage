#import <Foundation/Foundation.h>

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#else
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#endif

#import <QuartzCore/QuartzCore.h>
#import <CoreMedia/CoreMedia.h>

// GPUImageFramebuffer 管理者帧缓存和纹理附件。其中纹理附件涉及到了相关的纹理选项。因此，它提供的属性也是和帧缓存、纹理附件、纹理选项等相关。

typedef struct GPUTextureOptions {
    GLenum minFilter;
    GLenum magFilter;
    GLenum wrapS;
    GLenum wrapT;
    GLenum internalFormat;
    GLenum format;
    GLenum type;
} GPUTextureOptions;

@interface GPUImageFramebuffer : NSObject

// 帧缓存大小
@property(readonly) CGSize size;
// 纹理选项
@property(readonly) GPUTextureOptions textureOptions;
// 纹理缓存
@property(readonly) GLuint texture;
// 是否仅有纹理没有帧缓存
@property(readonly) BOOL missingFramebuffer;

// Initialization and teardown
- (id)initWithSize:(CGSize)framebufferSize;
- (id)initWithSize:(CGSize)framebufferSize textureOptions:(GPUTextureOptions)fboTextureOptions onlyTexture:(BOOL)onlyGenerateTexture;
- (id)initWithSize:(CGSize)framebufferSize overriddenTexture:(GLuint)inputTexture;

// Usage
// 与使用当前帧缓存相关的方法,激活
- (void)activateFramebuffer;

// Reference counting
// 与 GPUImageFramebuffer 引用计数相关的方法
- (void)lock;
- (void)unlock;
- (void)clearAllLocks;
- (void)disableReferenceCounting;
- (void)enableReferenceCounting;

// Image capture
// 从帧缓存生成位图相关的方法
- (CGImageRef)newCGImageFromFramebufferContents;
- (void)restoreRenderTarget;

// Raw data bytes
// 获取帧缓存原始数据相关的方法
- (void)lockForReading;
- (void)unlockAfterReading;
- (NSUInteger)bytesPerRow;
- (GLubyte *)byteBuffer;
- (CVPixelBufferRef)pixelBuffer;

@end
