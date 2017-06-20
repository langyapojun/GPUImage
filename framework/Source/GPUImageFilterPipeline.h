#import <Foundation/Foundation.h>
#import "GPUImageOutput.h"

@interface GPUImageFilterPipeline : NSObject
{
    NSString *stringValue;
}

//filter数组
@property (strong) NSMutableArray *filters;

//输入对象
@property (strong) GPUImageOutput *input;
//输出对象
@property (strong) id <GPUImageInput> output;

- (id) initWithOrderedFilters:(NSArray*) filters input:(GPUImageOutput*)input output:(id <GPUImageInput>)output;
- (id) initWithConfiguration:(NSDictionary*) configuration input:(GPUImageOutput*)input output:(id <GPUImageInput>)output;
- (id) initWithConfigurationFile:(NSURL*) configuration input:(GPUImageOutput*)input output:(id <GPUImageInput>)output;

// filter的增加、删除、替换
- (void) addFilter:(GPUImageOutput<GPUImageInput> *)filter;
- (void) addFilter:(GPUImageOutput<GPUImageInput> *)filter atIndex:(NSUInteger)insertIndex;
- (void) replaceFilterAtIndex:(NSUInteger)index withFilter:(GPUImageOutput<GPUImageInput> *)filter;
- (void) replaceAllFilters:(NSArray *) newFilters;
- (void) removeFilter:(GPUImageOutput<GPUImageInput> *)filter;
- (void) removeFilterAtIndex:(NSUInteger)index;
- (void) removeAllFilters;

// 由final filter生成图片
- (UIImage *) currentFilteredFrame;
- (UIImage *) currentFilteredFrameWithOrientation:(UIImageOrientation)imageOrientation;
- (CGImageRef) newCGImageFromCurrentFilteredFrame;

@end
