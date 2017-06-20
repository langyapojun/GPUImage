#import "GPUImageFilter.h"

extern NSString *const kGPUImageTwoInputTextureVertexShaderString;

/*!
 *  可以将两个帧缓存对象的输入合并成一个帧缓存对象的输出
 */
@interface GPUImageTwoInputFilter : GPUImageFilter
{
    // 第二个帧缓存对象的实例变量
    GPUImageFramebuffer *secondInputFramebuffer;

    // 与第二个帧缓存对象相关的参数
    GLint filterSecondTextureCoordinateAttribute;
    GLint filterInputTextureUniform2;
    GPUImageRotationMode inputRotation2;
    CMTime firstFrameTime, secondFrameTime;
    
    // 控制两个帧缓存对象渲染的相关参数
    BOOL hasSetFirstTexture, hasReceivedFirstFrame, hasReceivedSecondFrame, firstFrameWasVideo, secondFrameWasVideo;
    BOOL firstFrameCheckDisabled, secondFrameCheckDisabled;
}

// 需不需要检查第一个纹理输入
- (void)disableFirstFrameCheck;
// 需不需要检查第二个纹理输入
- (void)disableSecondFrameCheck;

@end
