#import "GPUImageOutput.h"

@interface GPUImageUIElement : GPUImageOutput

// Initialization and teardown
- (id)initWithView:(UIView *)inputView;
- (id)initWithLayer:(CALayer *)inputLayer;

// Layer management
// 获取像素大小
- (CGSize)layerSizeInPixels;
// 更新方法
- (void)update;
// 使用当前时间的更新方法
- (void)updateUsingCurrentTime;
// 带时间的更新方法
- (void)updateWithTimestamp:(CMTime)frameTime;

@end
