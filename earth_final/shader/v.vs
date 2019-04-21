#version 450 core                                                                               
                                                                                                
layout (location = 0) in vec3 Position_VS_in;                                                   
layout (location = 1) in vec2 TexCoord_VS_in;                                                     
                                                                                                
uniform mat4 model;                                                                            
                                                                                                
out vec3 WorldPos_CS_in;                                                                        
out vec2 TexCoord_CS_in;                                                                          
                                                                                                
void main()                                                                                     
{                                                                                               
    WorldPos_CS_in = (model * vec4(Position_VS_in, 1.0)).xyz;                                  
    TexCoord_CS_in = TexCoord_VS_in;                                                           
}