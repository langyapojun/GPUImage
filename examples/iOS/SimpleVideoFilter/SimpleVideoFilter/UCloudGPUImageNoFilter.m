//
//  UCloudGPUImageNoFilter.m
//  GPUImage
//
//  Created by Sidney on 16/06/16.
//  Copyright © 2016年 Brad Larson. All rights reserved.
//

#import "UCloudGPUImageNoFilter.h"

NSString *const kUCloudGPUImageNothingFragmentShaderString = UCloudSHADER_STRING
(
 varying highp vec2 textureCoordinate;
 
 uniform sampler2D inputImageTexture;
 uniform lowp float brightness;
 
 void main()
 {
     lowp vec4 textureColor = texture2D(inputImageTexture, textureCoordinate);
     
     gl_FragColor = textureColor;
 }
);

@implementation UCloudGPUImageNoFilter

- (id)init;
{
    if (!(self = [super initWithFragmentShaderFromString:kUCloudGPUImageNothingFragmentShaderString]))
    {
        return nil;
    }
    
    return self;
}

@end
