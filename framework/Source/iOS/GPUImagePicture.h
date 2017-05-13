#import <UIKit/UIKit.h>
#import "GPUImageOutput.h"

//将UIImage或CGImage转化为纹理对象。作为响应链源
/*
 直接alpha与预乘alpha
 
 使用直接 alpha 描述 RGBA 颜色时，颜色的 alpha 值会存储在 alpha 通道中。例如，若要描述具有 60% 不透明度的红色，使用以下值：(255, 0, 0, 255 * 0.6) = (255, 0, 0, 153)。其中153（153 = 255 * 0.6）指示颜色应具有 60% 的不透明度。
 
 使用预乘 alpha 描述 RGBA 颜色时，每种颜色都会与 alpha 值相乘：(255 * 0.6, 0 * 0.6, 0 * 0.6, 255 * 0.6) = (153, 0, 0, 153)。
 */

@interface GPUImagePicture : GPUImageOutput
{
    CGSize pixelSizeOfImage;
    BOOL hasProcessedImage;
    
    dispatch_semaphore_t imageUpdateSemaphore;
}

// Initialization and teardown
// 通过图片URL初始化
- (id)initWithURL:(NSURL *)url;
// 通过UIImage或CGImage初始化
- (id)initWithImage:(UIImage *)newImageSource;
- (id)initWithCGImage:(CGImageRef)newImageSource;
// 通过UIImage或CGImage、是否平滑缩放输出来初始化
- (id)initWithImage:(UIImage *)newImageSource smoothlyScaleOutput:(BOOL)smoothlyScaleOutput;
- (id)initWithCGImage:(CGImageRef)newImageSource smoothlyScaleOutput:(BOOL)smoothlyScaleOutput;
// 通过UIImage或CGImage、是否去除预乘alpha来初始化
- (id)initWithImage:(UIImage *)newImageSource removePremultiplication:(BOOL)removePremultiplication;
- (id)initWithCGImage:(CGImageRef)newImageSource removePremultiplication:(BOOL)removePremultiplication;
// 通过UIImage或CGImage、是否平滑缩放、是否去除预乘alpha来初始化
- (id)initWithImage:(UIImage *)newImageSource smoothlyScaleOutput:(BOOL)smoothlyScaleOutput removePremultiplication:(BOOL)removePremultiplication;
- (id)initWithCGImage:(CGImageRef)newImageSource smoothlyScaleOutput:(BOOL)smoothlyScaleOutput removePremultiplication:(BOOL)removePremultiplication;

// Image rendering
- (void)processImage;
- (CGSize)outputImageSize;

/**
 * Process image with all targets and filters asynchronously
 * The completion handler is called after processing finished in the
 * GPU's dispatch queue - and only if this method did not return NO.
 *
 * @returns NO if resource is blocked and processing is discarded, YES otherwise
 */
- (BOOL)processImageWithCompletionHandler:(void (^)(void))completion;
- (void)processImageUpToFilter:(GPUImageOutput<GPUImageInput> *)finalFilterInChain withCompletionHandler:(void (^)(UIImage *processedImage))block;

@end
