#import "GPUImageTwoInputFilter.h"

extern NSString *const kGPUImageThreeInputTextureVertexShaderString;

/*!
 *  可以将三个帧缓存对象的输入合并成一个帧缓存对象的输出
 */
@interface GPUImageThreeInputFilter : GPUImageTwoInputFilter
{
    // 第三个帧缓存对象的实例变量
    GPUImageFramebuffer *thirdInputFramebuffer;

    // 与第三个帧缓存对象相关的参数
    GLint filterThirdTextureCoordinateAttribute;
    GLint filterInputTextureUniform3;
    GPUImageRotationMode inputRotation3;
    GLuint filterSourceTexture3;
    CMTime thirdFrameTime;
    
    BOOL hasSetSecondTexture, hasReceivedThirdFrame, thirdFrameWasVideo;
    BOOL thirdFrameCheckDisabled;
}

// 需不需要检查第三个纹理输入
- (void)disableThirdFrameCheck;

@end
