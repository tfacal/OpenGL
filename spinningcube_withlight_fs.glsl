#version 410
struct Material {
   //vec3 ambient;
   sampler2D diffuse;
   sampler2D specular;
   float shininess;
 };

 struct Light {
   vec3 position;
   vec3 ambient;
   vec3 diffuse;
   vec3 specular;
 };

 out vec4 frag_col;

 in vec3 frag_3Dpos;
 in vec3 vs_normal;
 in vec2 vs_tex_coord;

 uniform Material material;
 uniform Light light;
 uniform Light light_second;
 uniform vec3 view_pos;

 void main() {
   // Ambient
   vec3 ambient = light.ambient * vec3(texture(material.diffuse, vs_tex_coord));

   vec3 light_dir = normalize(light.position - frag_3Dpos);

   // Diffuse
   float diff = max(dot(vs_normal, light_dir), 0.0);
   vec3 diffuse = light.diffuse * diff *  vec3(texture(material.diffuse, vs_tex_coord));;

   // Specular
   vec3 view_dir = normalize(view_pos - frag_3Dpos);
   vec3 reflect_dir = reflect(-light_dir, vs_normal);
   float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.shininess);
   vec3 specular = light.specular * spec * vec3(texture(material.specular,vs_tex_coord));

  // Ambient
   vec3 ambient_second = light_second.ambient * vec3(texture(material.diffuse, vs_tex_coord));

   vec3 light_dir_second = normalize(light_second.position - frag_3Dpos);

   // Diffuse
   float diff_second = max(dot(vs_normal, light_dir_second), 0.0);
   vec3 diffuse_second = light_second.diffuse * diff_second *  vec3(texture(material.diffuse, vs_tex_coord));

   // Specular
   vec3 view_dir_second = normalize(view_pos - frag_3Dpos);
   vec3 reflect_dir_second = reflect(-light_dir_second, vs_normal);
   float spec_second = pow(max(dot(view_dir_second, reflect_dir_second), 0.0), material.shininess);
   vec3 specular_second = light_second.specular * spec_second * vec3(texture(material.specular,vs_tex_coord));

   vec3 result = ambient + diffuse + specular + ambient_second + diffuse_second + specular_second;
   frag_col = vec4(result, 1.0);
 }