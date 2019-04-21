#version 450 core
out vec4 FragColor;                                                            
                                                                                            
in vec2 TexCoord_FS_in;                                                                     
in vec3 Normal_FS_in;                                                                       
in vec3 WorldPos_FS_in;  

uniform sampler2D diffuseMap;

uniform vec3 lightPos; 
uniform vec3 viewPos; 

void main()
{
    vec3 normal = normalize(Normal_FS_in);
   
    // get diffuse color
    vec3 color = texture(diffuseMap, TexCoord_FS_in).rgb;
    // ambient
    vec3 ambient = 0.1 * color;
    // diffuse
    vec3 lightDir = normalize(lightPos - WorldPos_FS_in);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;
    // specular
    vec3 viewDir = normalize(viewPos - WorldPos_FS_in);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    vec3 specular = vec3(0.2) * spec;
    FragColor = vec4(ambient + diffuse, 1.0);
    //FragColor = vec4(1.0,1.0,1.0, 1.0);
} 