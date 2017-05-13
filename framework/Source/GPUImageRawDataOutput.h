#import <Foundation/Foundation.h>
#import "GPUImageContext.h"

struct GPUByteColorVector {
    GLubyte red;
    GLubyte green;
    GLubyte blue;
    GLubyte alpha;
};
typedef struct GPUByteColorVector GPUByteColorVector;

@protocol GPUImageRawDataProcessor;

// 可以将输入的帧缓存转换为原始数据。
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
@interface GPUImageRawDataOutput : NSObject <GPUImageInput> {
    CGSize imageSize;
    GPUImageRotationMode inputRotation;
    BOOL outputBGRA;
}
#else
@interface GPUImageRawDataOutput : NSObject <GPUImageInput> {
    CGSize imageSize;
    GPUImageRotationMode inputRotation;
    BOOL outputBGRA;
}
#endif

@property(readonly) GLubyte *rawBytesForImage;
@property(nonatomic, copy) void(^newFrameAvailableBlock)(void);
@property(nonatomic) BOOL enabled;

// Initialization and teardown
// 构造方法最主要的任务是构造GL程序。
- (id)initWithImageSize:(CGSize)newImageSize resultsInBGRAFormat:(BOOL)resultsInBGRAFormat;

// Data access
// 获取特定位置的像素向量
- (GPUByteColorVector)colorAtLocation:(CGPoint)locationInImage;
// 每行数据大小
- (NSUInteger)bytesPerRowInOutput;
// 设置纹理大小
- (void)setImageSize:(CGSize)newImageSize;
// 锁定、与解锁帧缓存
- (void)lockFramebufferForReading;
- (void)unlockFramebufferAfterReading;

@end
