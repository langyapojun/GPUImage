#import "GPUImageFramebufferCache.h"
#import "GPUImageContext.h"
#import "GPUImageOutput.h"

#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#endif

@interface GPUImageFramebufferCache()
{
//    NSCache *framebufferCache;
    // 缓存字典
    NSMutableDictionary *framebufferCache;
    // 缓存数量字典
    NSMutableDictionary *framebufferTypeCounts;
    // 当前正在使用的GPUImageFramebuffer数组
    NSMutableArray *activeImageCaptureList; // Where framebuffers that may be lost by a filter, but which are still needed for a UIImage, etc., are stored
    id memoryWarningObserver;

    // 缓存队列
    dispatch_queue_t framebufferCacheQueue;
}

- (NSString *)hashForSize:(CGSize)size textureOptions:(GPUTextureOptions)textureOptions onlyTexture:(BOOL)onlyTexture;

@end


@implementation GPUImageFramebufferCache

#pragma mark -
#pragma mark Initialization and teardown

- (id)init;
{
    if (!(self = [super init]))
    {
		return nil;
    }
    
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
    __unsafe_unretained __typeof__ (self) weakSelf = self;
    // 监听系统该内存警告，收到通知，便清理缓存数组
    memoryWarningObserver = [[NSNotificationCenter defaultCenter] addObserverForName:UIApplicationDidReceiveMemoryWarningNotification object:nil queue:nil usingBlock:^(NSNotification *note) {
        __typeof__ (self) strongSelf = weakSelf;
        if (strongSelf) {
            [strongSelf purgeAllUnassignedFramebuffers];
        }
    }];
#else
#endif

    // 初始化缓冲池
//    framebufferCache = [[NSCache alloc] init];
    framebufferCache = [[NSMutableDictionary alloc] init];
    framebufferTypeCounts = [[NSMutableDictionary alloc] init];
    activeImageCaptureList = [[NSMutableArray alloc] init];
    framebufferCacheQueue = dispatch_queue_create("com.sunsetlakesoftware.GPUImage.framebufferCacheQueue", GPUImageDefaultQueueAttribute());
    
    return self;
}

- (void)dealloc;
{
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
    [[NSNotificationCenter defaultCenter] removeObserver:self];
#else
#endif
}

#pragma mark -
#pragma mark Framebuffer management

- (NSString *)hashForSize:(CGSize)size textureOptions:(GPUTextureOptions)textureOptions onlyTexture:(BOOL)onlyTexture;
{
    if (onlyTexture)
    {
        return [NSString stringWithFormat:@"%.1fx%.1f-%d:%d:%d:%d:%d:%d:%d-NOFB", size.width, size.height, textureOptions.minFilter, textureOptions.magFilter, textureOptions.wrapS, textureOptions.wrapT, textureOptions.internalFormat, textureOptions.format, textureOptions.type];
    }
    else
    {
        return [NSString stringWithFormat:@"%.1fx%.1f-%d:%d:%d:%d:%d:%d:%d", size.width, size.height, textureOptions.minFilter, textureOptions.magFilter, textureOptions.wrapS, textureOptions.wrapT, textureOptions.internalFormat, textureOptions.format, textureOptions.type];
    }
}

- (GPUImageFramebuffer *)fetchFramebufferForSize:(CGSize)framebufferSize textureOptions:(GPUTextureOptions)textureOptions onlyTexture:(BOOL)onlyTexture;
{
    __block GPUImageFramebuffer *framebufferFromCache = nil;
//    dispatch_sync(framebufferCacheQueue, ^{
    runSynchronouslyOnVideoProcessingQueue(^{
        
        // 创建查找字符串
        NSString *lookupHash = [self hashForSize:framebufferSize textureOptions:textureOptions onlyTexture:onlyTexture];
        // 获取GPUImageFramebuffer在缓存中的数量
        NSNumber *numberOfMatchingTexturesInCache = [framebufferTypeCounts objectForKey:lookupHash];
        NSInteger numberOfMatchingTextures = [numberOfMatchingTexturesInCache integerValue];
        
        // 缓存中如果没有，则创建
        if ([numberOfMatchingTexturesInCache integerValue] < 1)
        {
            // Nothing in the cache, create a new framebuffer to use
            framebufferFromCache = [[GPUImageFramebuffer alloc] initWithSize:framebufferSize textureOptions:textureOptions onlyTexture:onlyTexture];
        }
        else
        {
            // Something found, pull the old framebuffer and decrement the count
            // 缓存中如果有，则去除最后一个，如果去除framebufferFromCache为空，则取倒数第二个，以此类推
            NSInteger currentTextureID = (numberOfMatchingTextures - 1);
            while ((framebufferFromCache == nil) && (currentTextureID >= 0))
            {
                // 根据数量构建带数量的textureHash字符串
                NSString *textureHash = [NSString stringWithFormat:@"%@-%ld", lookupHash, (long)currentTextureID];
                
                //查找以textureHash为key的GPUImageFramebuffer是否存在
                framebufferFromCache = [framebufferCache objectForKey:textureHash];
                // Test the values in the cache first, to see if they got invalidated behind our back
                if (framebufferFromCache != nil)
                {
                    // 存在，则从缓存中删除
                    // Withdraw this from the cache while it's in use
                    [framebufferCache removeObjectForKey:textureHash];
                }
                currentTextureID--;
            }
            
            currentTextureID++;
            
            // 更新framebufferTypeCounts中相同类型GPUImageFramebuffer的数量
            [framebufferTypeCounts setObject:[NSNumber numberWithInteger:currentTextureID] forKey:lookupHash];
            
            // 还是没有则创建
            if (framebufferFromCache == nil)
            {
                framebufferFromCache = [[GPUImageFramebuffer alloc] initWithSize:framebufferSize textureOptions:textureOptions onlyTexture:onlyTexture];
            }
        }
    });

    // 引用计数加一，返回
    [framebufferFromCache lock];
    return framebufferFromCache;
}

// 根据framebufferSize和onlyTexture以及默认的GPUTextureOptions查找GPUImageFramebuffer。如果找不到，会创建新的缓存。
- (GPUImageFramebuffer *)fetchFramebufferForSize:(CGSize)framebufferSize onlyTexture:(BOOL)onlyTexture;
{
    GPUTextureOptions defaultTextureOptions;
    defaultTextureOptions.minFilter = GL_LINEAR;
    defaultTextureOptions.magFilter = GL_LINEAR;
    defaultTextureOptions.wrapS = GL_CLAMP_TO_EDGE;
    defaultTextureOptions.wrapT = GL_CLAMP_TO_EDGE;
    defaultTextureOptions.internalFormat = GL_RGBA;
    defaultTextureOptions.format = GL_BGRA;
    defaultTextureOptions.type = GL_UNSIGNED_BYTE;
    
    return [self fetchFramebufferForSize:framebufferSize textureOptions:defaultTextureOptions onlyTexture:onlyTexture];
}

// 回收缓存。根据size、textureOptions和onlyTexture，创建缓存的key值，ramebufferTypeCounts中的key由lookupHash构成没有加数量。在framebufferCache中，key值由lookupHash加上数量避免覆盖相同的GPUImageFramebuffer。
- (void)returnFramebufferToCache:(GPUImageFramebuffer *)framebuffer;
{
    // 清除引用计数
    [framebuffer clearAllLocks];
    
//    dispatch_async(framebufferCacheQueue, ^{
    runAsynchronouslyOnVideoProcessingQueue(^{
        CGSize framebufferSize = framebuffer.size;
        GPUTextureOptions framebufferTextureOptions = framebuffer.textureOptions;
        
        // 常见查找hash字符串
        NSString *lookupHash = [self hashForSize:framebufferSize textureOptions:framebufferTextureOptions onlyTexture:framebuffer.missingFramebuffer];
        // 获取当前同类型缓存的数量
        NSNumber *numberOfMatchingTexturesInCache = [framebufferTypeCounts objectForKey:lookupHash];
        NSInteger numberOfMatchingTextures = [numberOfMatchingTexturesInCache integerValue];
        
        // 对相同类型的GPUImageFramebuffer,存放在framebufferCache中时，key值由lookupHash加上数量避免覆盖相同的GPUImageFramebuffer。
        NSString *textureHash = [NSString stringWithFormat:@"%@-%ld", lookupHash, (long)numberOfMatchingTextures];
        
//        [framebufferCache setObject:framebuffer forKey:textureHash cost:round(framebufferSize.width * framebufferSize.height * 4.0)];
        [framebufferCache setObject:framebuffer forKey:textureHash];
        
        // framebufferTypeCounts中的key没有加数量
        [framebufferTypeCounts setObject:[NSNumber numberWithInteger:(numberOfMatchingTextures + 1)] forKey:lookupHash];
    });
}

// 内存警告的时候，清空缓存
- (void)purgeAllUnassignedFramebuffers;
{
    runAsynchronouslyOnVideoProcessingQueue(^{
//    dispatch_async(framebufferCacheQueue, ^{
        [framebufferCache removeAllObjects];
        [framebufferTypeCounts removeAllObjects];
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
        CVOpenGLESTextureCacheFlush([[GPUImageContext sharedImageProcessingContext] coreVideoTextureCache], 0);
#else
#endif
    });
}

// 帧缓存持有与释放。在读取帧缓存图像数据时，需要保持对GPUImageFramebuffer的引用。并且读取完数据后，需要对其进行释放。详细见GPUImageFramebuffer的newCGImageFromFramebufferContents方法。
- (void)addFramebufferToActiveImageCaptureList:(GPUImageFramebuffer *)framebuffer;
{
    runAsynchronouslyOnVideoProcessingQueue(^{
//    dispatch_async(framebufferCacheQueue, ^{
        [activeImageCaptureList addObject:framebuffer];
    });
}

- (void)removeFramebufferFromActiveImageCaptureList:(GPUImageFramebuffer *)framebuffer;
{
    runAsynchronouslyOnVideoProcessingQueue(^{
//  dispatch_async(framebufferCacheQueue, ^{
        [activeImageCaptureList removeObject:framebuffer];
    });
}

@end
