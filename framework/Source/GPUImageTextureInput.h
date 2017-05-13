#import "GPUImageOutput.h"

// 可以用传入的纹理生成帧缓存对象。因此，可以作为响应链源使用。
@interface GPUImageTextureInput : GPUImageOutput
{
    CGSize textureSize;
}

// Initialization and teardown
/**
 构造方法

 @param newInputTexture 纹理对象
 @param newTextureSize 纹理大小
 @return GPUImageTextureInput
 */
- (id)initWithTexture:(GLuint)newInputTexture size:(CGSize)newTextureSize;

// Image rendering
- (void)processTextureWithFrameTime:(CMTime)frameTime;

@end
