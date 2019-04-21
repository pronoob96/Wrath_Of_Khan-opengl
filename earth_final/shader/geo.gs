#version 450 core

layout(triangles) in;
layout (triangle_strip, max_vertices=3) out;

in vec3 WorldPos_GS_in[3];                                                                        
in vec2 TexCoord_GS_in[3];    

out vec3 WorldPos_FS_in;                                                                        
out vec2 TexCoord_FS_in;                                                                        
out vec3 Normal_FS_in;

 void main()
 {

        vec3 n = cross(WorldPos_GS_in[1].xyz-WorldPos_GS_in[0].xyz, WorldPos_GS_in[2].xyz-WorldPos_GS_in[0].xyz);
        for(int i = 0; i < gl_in.length(); i++)
        {
             gl_Position = gl_in[i].gl_Position;

             TexCoord_FS_in = TexCoord_GS_in[i];
             Normal_FS_in = n;
             WorldPos_FS_in = WorldPos_GS_in[i];

             EmitVertex();
        }
}