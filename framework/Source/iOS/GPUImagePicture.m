#import "GPUImagePicture.h"

@implementation GPUImagePicture

#pragma mark -
#pragma mark Initialization and teardown

- (id)initWithURL:(NSURL *)url;
{
    NSData *imageData = [[NSData alloc] initWithContentsOfURL:url];
    
    if (!(self = [self initWithData:imageData]))
    {
        return nil;
    }
    
    return self;
}

- (id)initWithData:(NSData *)imageData;
{
    UIImage *inputImage = [[UIImage alloc] initWithData:imageData];
    
    if (!(self = [self initWithImage:inputImage]))
    {
		return nil;
    }
    
    return self;
}

- (id)initWithImage:(UIImage *)newImageSource;
{
    if (!(self = [self initWithImage:newImageSource smoothlyScaleOutput:NO]))
    {
		return nil;
    }
    
    return self;
}

- (id)initWithCGImage:(CGImageRef)newImageSource;
{
    if (!(self = [self initWithCGImage:newImageSource smoothlyScaleOutput:NO]))
    {
		return nil;
    }
    return self;
}

- (id)initWithImage:(UIImage *)newImageSource smoothlyScaleOutput:(BOOL)smoothlyScaleOutput;
{
    return [self initWithCGImage:[newImageSource CGImage] smoothlyScaleOutput:smoothlyScaleOutput];
}

- (id)initWithCGImage:(CGImageRef)newImageSource smoothlyScaleOutput:(BOOL)smoothlyScaleOutput;
{
    return [self initWithCGImage:newImageSource smoothlyScaleOutput:smoothlyScaleOutput removePremultiplication:NO];
}

- (id)initWithImage:(UIImage *)newImageSource removePremultiplication:(BOOL)removePremultiplication;
{
    return [self initWithCGImage:[newImageSource CGImage] smoothlyScaleOutput:NO removePremultiplication:removePremultiplication];
}

- (id)initWithCGImage:(CGImageRef)newImageSource removePremultiplication:(BOOL)removePremultiplication;
{
    return [self initWithCGImage:newImageSource smoothlyScaleOutput:NO removePremultiplication:removePremultiplication];
}

- (id)initWithImage:(UIImage *)newImageSource smoothlyScaleOutput:(BOOL)smoothlyScaleOutput removePremultiplication:(BOOL)removePremultiplication;
{
    return [self initWithCGImage:[newImageSource CGImage] smoothlyScaleOutput:smoothlyScaleOutput removePremultiplication:removePremultiplication];
}

/*
 基本思路就是：
 1、获取图片适合的宽高（不能超出OpenGL ES允许的最大纹理宽高）
 2、如果使用了smoothlyScaleOutput，需要调整宽高为接近2的幂的值，调整后必须重绘；
 3、如果不用重绘，则获取大小端、alpha等信息；
 4、需要重绘，则使用CoreGraphics重绘；
 5、根据是否需要去除预乘alpha选项，决定是否去除预乘alpha；
 6、由调整后的数据生成纹理缓存对象；
 7、根据shouldSmoothlyScaleOutput选项决定是否生成mipmaps；
 8、最后释放资源。
 */
- (id)initWithCGImage:(CGImageRef)newImageSource smoothlyScaleOutput:(BOOL)smoothlyScaleOutput removePremultiplication:(BOOL)removePremultiplication;
{
    if (!(self = [super init]))
    {
		return nil;
    }
    
    hasProcessedImage = NO;
    self.shouldSmoothlyScaleOutput = smoothlyScaleOutput;
    imageUpdateSemaphore = dispatch_semaphore_create(0);
    dispatch_semaphore_signal(imageUpdateSemaphore);


    // TODO: Dispatch this whole thing asynchronously to move image loading off main thread
    CGFloat widthOfImage = CGImageGetWidth(newImageSource);
    CGFloat heightOfImage = CGImageGetHeight(newImageSource);

    // If passed an empty image reference, CGContextDrawImage will fail in future versions of the SDK.
    NSAssert( widthOfImage > 0 && heightOfImage > 0, @"Passed image must not be empty - it should be at least 1px tall and wide");
    
    pixelSizeOfImage = CGSizeMake(widthOfImage, heightOfImage);
    CGSize pixelSizeToUseForTexture = pixelSizeOfImage;
    
    BOOL shouldRedrawUsingCoreGraphics = NO;
    
    // For now, deal with images larger than the maximum texture size by resizing to be within that limit
    CGSize scaledImageSizeToFitOnGPU = [GPUImageContext sizeThatFitsWithinATextureForSize:pixelSizeOfImage];
    if (!CGSizeEqualToSize(scaledImageSizeToFitOnGPU, pixelSizeOfImage))
    {
        pixelSizeOfImage = scaledImageSizeToFitOnGPU;
        pixelSizeToUseForTexture = pixelSizeOfImage;
        shouldRedrawUsingCoreGraphics = YES;
    }
    
    if (self.shouldSmoothlyScaleOutput)
    {
        // In order to use mipmaps, you need to provide power-of-two textures, so convert to the next largest power of two and stretch to fill
        CGFloat powerClosestToWidth = ceil(log2(pixelSizeOfImage.width));
        CGFloat powerClosestToHeight = ceil(log2(pixelSizeOfImage.height));
        
        pixelSizeToUseForTexture = CGSizeMake(pow(2.0, powerClosestToWidth), pow(2.0, powerClosestToHeight));
        
        shouldRedrawUsingCoreGraphics = YES;
    }
    
    GLubyte *imageData = NULL;
    CFDataRef dataFromImageDataProvider = NULL;
    GLenum format = GL_BGRA;
    BOOL isLitteEndian = YES;
    BOOL alphaFirst = NO;
    BOOL premultiplied = NO;
	
    if (!shouldRedrawUsingCoreGraphics) {
        /* Check that the memory layout is compatible with GL, as we cannot use glPixelStore to
         * tell GL about the memory layout with GLES.
         */
        if (CGImageGetBytesPerRow(newImageSource) != CGImageGetWidth(newImageSource) * 4 ||
            CGImageGetBitsPerPixel(newImageSource) != 32 ||
            CGImageGetBitsPerComponent(newImageSource) != 8)
        {
            shouldRedrawUsingCoreGraphics = YES;
        } else {
            /* Check that the bitmap pixel format is compatible with GL */
            CGBitmapInfo bitmapInfo = CGImageGetBitmapInfo(newImageSource);
            if ((bitmapInfo & kCGBitmapFloatComponents) != 0) {
                /* We don't support float components for use directly in GL */
                shouldRedrawUsingCoreGraphics = YES;
            } else {
                CGBitmapInfo byteOrderInfo = bitmapInfo & kCGBitmapByteOrderMask;
                if (byteOrderInfo == kCGBitmapByteOrder32Little) {
                    /* Little endian, for alpha-first we can use this bitmap directly in GL */
                    CGImageAlphaInfo alphaInfo = bitmapInfo & kCGBitmapAlphaInfoMask;
                    if (alphaInfo != kCGImageAlphaPremultipliedFirst && alphaInfo != kCGImageAlphaFirst &&
                        alphaInfo != kCGImageAlphaNoneSkipFirst) {
                        shouldRedrawUsingCoreGraphics = YES;
                    }
                } else if (byteOrderInfo == kCGBitmapByteOrderDefault || byteOrderInfo == kCGBitmapByteOrder32Big) {
					isLitteEndian = NO;
                    /* Big endian, for alpha-last we can use this bitmap directly in GL */
                    CGImageAlphaInfo alphaInfo = bitmapInfo & kCGBitmapAlphaInfoMask;
                    if (alphaInfo != kCGImageAlphaPremultipliedLast && alphaInfo != kCGImageAlphaLast &&
                        alphaInfo != kCGImageAlphaNoneSkipLast) {
                        shouldRedrawUsingCoreGraphics = YES;
                    } else {
                        /* Can access directly using GL_RGBA pixel format */
						premultiplied = alphaInfo == kCGImageAlphaPremultipliedLast || alphaInfo == kCGImageAlphaPremultipliedLast;
						alphaFirst = alphaInfo == kCGImageAlphaFirst || alphaInfo == kCGImageAlphaPremultipliedFirst;
						format = GL_RGBA;
                    }
                }
            }
        }
    }
    
    //    CFAbsoluteTime elapsedTime, startTime = CFAbsoluteTimeGetCurrent();
    
    if (shouldRedrawUsingCoreGraphics)
    {
        // For resized or incompatible image: redraw
        imageData = (GLubyte *) calloc(1, (int)pixelSizeToUseForTexture.width * (int)pixelSizeToUseForTexture.height * 4);
        
        CGColorSpaceRef genericRGBColorspace = CGColorSpaceCreateDeviceRGB();
        
        CGContextRef imageContext = CGBitmapContextCreate(imageData, (size_t)pixelSizeToUseForTexture.width, (size_t)pixelSizeToUseForTexture.height, 8, (size_t)pixelSizeToUseForTexture.width * 4, genericRGBColorspace,  kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
        //        CGContextSetBlendMode(imageContext, kCGBlendModeCopy); // From Technical Q&A QA1708: http://developer.apple.com/library/ios/#qa/qa1708/_index.html
        CGContextDrawImage(imageContext, CGRectMake(0.0, 0.0, pixelSizeToUseForTexture.width, pixelSizeToUseForTexture.height), newImageSource);
        CGContextRelease(imageContext);
        CGColorSpaceRelease(genericRGBColorspace);
		isLitteEndian = YES;
		alphaFirst = YES;
		premultiplied = YES;
    }
    else
    {
        // Access the raw image bytes directly
        dataFromImageDataProvider = CGDataProviderCopyData(CGImageGetDataProvider(newImageSource));
        imageData = (GLubyte *)CFDataGetBytePtr(dataFromImageDataProvider);
    }
	
	if (removePremultiplication && premultiplied) {
		NSUInteger	totalNumberOfPixels = round(pixelSizeToUseForTexture.width * pixelSizeToUseForTexture.height);
		uint32_t	*pixelP = (uint32_t *)imageData;
		uint32_t	pixel;
		CGFloat		srcR, srcG, srcB, srcA;

		for (NSUInteger idx=0; idx<totalNumberOfPixels; idx++, pixelP++) {
			pixel = isLitteEndian ? CFSwapInt32LittleToHost(*pixelP) : CFSwapInt32BigToHost(*pixelP);

			if (alphaFirst) {
				srcA = (CGFloat)((pixel & 0xff000000) >> 24) / 255.0f;
			}
			else {
				srcA = (CGFloat)(pixel & 0x000000ff) / 255.0f;
				pixel >>= 8;
			}

			srcR = (CGFloat)((pixel & 0x00ff0000) >> 16) / 255.0f;
			srcG = (CGFloat)((pixel & 0x0000ff00) >> 8) / 255.0f;
			srcB = (CGFloat)(pixel & 0x000000ff) / 255.0f;
			
			srcR /= srcA; srcG /= srcA; srcB /= srcA;
			
			pixel = (uint32_t)(srcR * 255.0) << 16;
			pixel |= (uint32_t)(srcG * 255.0) << 8;
			pixel |= (uint32_t)(srcB * 255.0);

			if (alphaFirst) {
				pixel |= (uint32_t)(srcA * 255.0) << 24;
			}
			else {
				pixel <<= 8;
				pixel |= (uint32_t)(srcA * 255.0);
			}
			*pixelP = isLitteEndian ? CFSwapInt32HostToLittle(pixel) : CFSwapInt32HostToBig(pixel);
		}
	}
	
    //    elapsedTime = (CFAbsoluteTimeGetCurrent() - startTime) * 1000.0;
    //    NSLog(@"Core Graphics drawing time: %f", elapsedTime);
    
    //    CGFloat currentRedTotal = 0.0f, currentGreenTotal = 0.0f, currentBlueTotal = 0.0f, currentAlphaTotal = 0.0f;
    //	NSUInteger totalNumberOfPixels = round(pixelSizeToUseForTexture.width * pixelSizeToUseForTexture.height);
    //
    //    for (NSUInteger currentPixel = 0; currentPixel < totalNumberOfPixels; currentPixel++)
    //    {
    //        currentBlueTotal += (CGFloat)imageData[(currentPixel * 4)] / 255.0f;
    //        currentGreenTotal += (CGFloat)imageData[(currentPixel * 4) + 1] / 255.0f;
    //        currentRedTotal += (CGFloat)imageData[(currentPixel * 4 + 2)] / 255.0f;
    //        currentAlphaTotal += (CGFloat)imageData[(currentPixel * 4) + 3] / 255.0f;
    //    }
    //
    //    NSLog(@"Debug, average input image red: %f, green: %f, blue: %f, alpha: %f", currentRedTotal / (CGFloat)totalNumberOfPixels, currentGreenTotal / (CGFloat)totalNumberOfPixels, currentBlueTotal / (CGFloat)totalNumberOfPixels, currentAlphaTotal / (CGFloat)totalNumberOfPixels);
    
    runSynchronouslyOnVideoProcessingQueue(^{
        [GPUImageContext useImageProcessingContext];
        
        outputFramebuffer = [[GPUImageContext sharedFramebufferCache] fetchFramebufferForSize:pixelSizeToUseForTexture onlyTexture:YES];
        [outputFramebuffer disableReferenceCounting];

        glBindTexture(GL_TEXTURE_2D, [outputFramebuffer texture]);
        if (self.shouldSmoothlyScaleOutput)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        // no need to use self.outputTextureOptions here since pictures need this texture formats and type
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)pixelSizeToUseForTexture.width, (int)pixelSizeToUseForTexture.height, 0, format, GL_UNSIGNED_BYTE, imageData);
        
        if (self.shouldSmoothlyScaleOutput)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    });
    
    if (shouldRedrawUsingCoreGraphics)
    {
        free(imageData);
    }
    else
    {
        if (dataFromImageDataProvider)
        {
            CFRelease(dataFromImageDataProvider);
        }
    }
    
    return self;
}

// ARC forbids explicit message send of 'release'; since iOS 6 even for dispatch_release() calls: stripping it out in that case is required.
- (void)dealloc;
{
    [outputFramebuffer enableReferenceCounting];
    [outputFramebuffer unlock];

#if !OS_OBJECT_USE_OBJC
    if (imageUpdateSemaphore != NULL)
    {
        dispatch_release(imageUpdateSemaphore);
    }
#endif
}

#pragma mark -
#pragma mark Image rendering

- (void)removeAllTargets;
{
    [super removeAllTargets];
    hasProcessedImage = NO;
}

// 处理图片
- (void)processImage;
{
    [self processImageWithCompletionHandler:nil];
}

// 处理图片，可以传入处理完回调的block
- (BOOL)processImageWithCompletionHandler:(void (^)(void))completion;
{
    hasProcessedImage = YES;
    
    //    dispatch_semaphore_wait(imageUpdateSemaphore, DISPATCH_TIME_FOREVER);
     // 如果计数器小于1便立即返回。计数器大于等于1的时候，使计数器减1，并且往下执行
    if (dispatch_semaphore_wait(imageUpdateSemaphore, DISPATCH_TIME_NOW) != 0)
    {
        return NO;
    }
    
    // 传递Framebuffer给所有targets处理
    runAsynchronouslyOnVideoProcessingQueue(^{        
        for (id<GPUImageInput> currentTarget in targets)
        {
            NSInteger indexOfObject = [targets indexOfObject:currentTarget];
            NSInteger textureIndexOfTarget = [[targetTextureIndices objectAtIndex:indexOfObject] integerValue];
            
            [currentTarget setCurrentlyReceivingMonochromeInput:NO];
            [currentTarget setInputSize:pixelSizeOfImage atIndex:textureIndexOfTarget];
            [currentTarget setInputFramebuffer:outputFramebuffer atIndex:textureIndexOfTarget];
            [currentTarget newFrameReadyAtTime:kCMTimeIndefinite atIndex:textureIndexOfTarget];
        }
        
        // 执行完，计数器加1
        dispatch_semaphore_signal(imageUpdateSemaphore);
        
        // 有block，执行block
        if (completion != nil) {
            completion();
        }
    });
    
    return YES;
}

// 由响应链的final filter生成UIImage图像
- (void)processImageUpToFilter:(GPUImageOutput<GPUImageInput> *)finalFilterInChain withCompletionHandler:(void (^)(UIImage *processedImage))block;
{
    [finalFilterInChain useNextFrameForImageCapture];
    [self processImageWithCompletionHandler:^{
        UIImage *imageFromFilter = [finalFilterInChain imageFromCurrentFramebuffer];
        block(imageFromFilter);
    }];
}

// 输出图片大小，由于图像大小可能被调整（详见初始化方法）。因此，提供了获取图像大小的方法
- (CGSize)outputImageSize;
{
    return pixelSizeOfImage;
}

- (void)addTarget:(id<GPUImageInput>)newTarget atTextureLocation:(NSInteger)textureLocation;
{
    [super addTarget:newTarget atTextureLocation:textureLocation];
    
    if (hasProcessedImage)
    {
        [newTarget setInputSize:pixelSizeOfImage atIndex:textureLocation];
        [newTarget newFrameReadyAtTime:kCMTimeIndefinite atIndex:textureLocation];
    }
}

@end
