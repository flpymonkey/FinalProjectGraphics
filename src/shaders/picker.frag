#version 330 core

// Ouput data
out vec4 fragment_color;

// Values that stay constant for the whole mesh.
uniform vec4 PickingColor;

void main(){

    fragment_color = PickingColor;

}
