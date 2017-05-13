#import "GPUImageOutput.h"

// The bytes passed into this input are not copied or retained, but you are free to deallocate them after they are used by this filter.
// The bytes are uploaded and stored within a texture, so nothing is kept locally.
// The default format for input bytes is GPUPixelFormatBGRA, unless specified with pixelFormat:
// The default type for input bytes is GPUPixelTypeUByte, unless specified with pixelType:

typedef enum {
	GPUPixelFormatBGRA = GL_BGRA,
	GPUPixelFormatRGBA = GL_RGBA,
	GPUPixelFormatRGB = GL_RGB,
    GPUPixelFormatLuminance = GL_LUMINANCE
} GPUPixelFormat;

typedef enum {
	GPUPixelTypeUByte = GL_UNSIGNED_BYTE,
	GPUPixelTypeFloat = GL_FLOAT
} GPUPixelType;

// 接受RawData输入（包括：RGB、RGBA、BGRA、LUMINANCE数据）并生成帧缓存对象。
@interface GPUImageRawDataInput : GPUImageOutput
{
    CGSize uploadedImageSize;
	
	dispatch_semaphore_t dataUpdateSemaphore;
}

// Initialization and teardown
// 构造方法。构造的时候主要是根据RawData数据指针，数据大小，以及数据格式进行构造。
/*  构造的时候如果不指定像素格式和数据类型，默认会使用GPUPixelFormatBGRA和GPUPixelTypeUByte的方式。
    构造的过程是：
        1、初始化实例变量；
        2、生成只有纹理对象的GPUImageFramebuffer对象；
        3、给纹理对象绑定RawData数据。
 */
- (id)initWithBytes:(GLubyte *)bytesToUpload size:(CGSize)imageSize;
- (id)initWithBytes:(GLubyte *)bytesToUpload size:(CGSize)imageSize pixelFormat:(GPUPixelFormat)pixelFormat;
- (id)initWithBytes:(GLubyte *)bytesToUpload size:(CGSize)imageSize pixelFormat:(GPUPixelFormat)pixelFormat type:(GPUPixelType)pixelType;

/** Input data pixel format
 */
@property (readwrite, nonatomic) GPUPixelFormat pixelFormat;
@property (readwrite, nonatomic) GPUPixelType   pixelType;

// Image rendering
// 上传原始数据
- (void)updateDataFromBytes:(GLubyte *)bytesToUpload size:(CGSize)imageSize;
// 数据处理
- (void)processData;
- (void)processDataForTimestamp:(CMTime)frameTime;
//输出纹理大小
- (CGSize)outputImageSize;

@end
