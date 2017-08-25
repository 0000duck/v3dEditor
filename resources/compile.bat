@echo off

set SHADER_COMPILER=%VK_SDK_PATH%\Bin\glslangValidator.exe
set OBJ_DIR=obj\shaders

if NOT EXIST "%OBJ_DIR%" (
	md "%OBJ_DIR%"
)

%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\imgui_vert.spv" "shaders\imgui.vert"
%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\imgui_frag.spv" "shaders\imgui.frag"

%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\simple_vert.spv" "shaders\simple.vert"
%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\simple_frag.spv" "shaders\simple.frag"

%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\grid_vert.spv" "shaders\grid.vert"
%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\grid_frag.spv" "shaders\grid.frag"

%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\directionalLighting_o_frag.spv" "shaders\directionalLighting_o.frag"
%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\finishLighting_o_frag.spv" "shaders\finishLighting_o.frag"

%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\ssao_frag.spv" "shaders\ssao.frag"
%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\selectMapping_frag.spv" "shaders\selectMapping.frag"
%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\toneMapping_frag.spv" "shaders\toneMapping.frag"
%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\fxaa_frag.spv" "shaders\fxaa.frag"

%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\debug_vert.spv" "shaders\debug.vert"
%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\debug_frag.spv" "shaders\debug.frag"

%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\paste_frag.spv" "shaders\paste.frag"
%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\brightPass_frag.spv" "shaders\brightPass.frag"
%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\downSampling2x2_frag.spv" "shaders\downSampling2x2.frag"
%SHADER_COMPILER% -V -l -o "%OBJ_DIR%\gaussianBlur_frag.spv" "shaders\gaussianBlur.frag"

BinToTxt /i "%OBJ_DIR%\imgui_vert.spv" /o "..\source\v3dEditor\resource\gui_vert_spv.inc"
BinToTxt /i "%OBJ_DIR%\imgui_frag.spv" /o "..\source\v3dEditor\resource\gui_frag_spv.inc"

BinToTxt /i "%OBJ_DIR%\simple_vert.spv" /o "..\source\v3dEditor\resource\simple_vert_spv.inc"
BinToTxt /i "%OBJ_DIR%\simple_frag.spv" /o "..\source\v3dEditor\resource\simple_frag_spv.inc"

BinToTxt /i "%OBJ_DIR%\grid_vert.spv" /o "..\source\v3dEditor\resource\grid_vert_spv.inc"
BinToTxt /i "%OBJ_DIR%\grid_frag.spv" /o "..\source\v3dEditor\resource\grid_frag_spv.inc"

BinToTxt /i "%OBJ_DIR%\directionalLighting_o_frag.spv" /o "..\source\v3dEditor\resource\directionalLighting_o_frag_spv.inc"
BinToTxt /i "%OBJ_DIR%\finishLighting_o_frag.spv" /o "..\source\v3dEditor\resource\finishLighting_o_frag_spv.inc"

BinToTxt /i "%OBJ_DIR%\ssao_frag.spv" /o "..\source\v3dEditor\resource\ssao_frag_spv.inc"
BinToTxt /i "%OBJ_DIR%\selectMapping_frag.spv" /o "..\source\v3dEditor\resource\selectMapping_frag_spv.inc"
BinToTxt /i "%OBJ_DIR%\toneMapping_frag.spv" /o "..\source\v3dEditor\resource\toneMapping_frag_spv.inc"
BinToTxt /i "%OBJ_DIR%\fxaa_frag.spv" /o "..\source\v3dEditor\resource\fxaa_frag_spv.inc"

BinToTxt /i "%OBJ_DIR%\debug_vert.spv" /o "..\source\v3dEditor\resource\debug_vert_spv.inc"
BinToTxt /i "%OBJ_DIR%\debug_frag.spv" /o "..\source\v3dEditor\resource\debug_frag_spv.inc"

BinToTxt /i "%OBJ_DIR%\paste_frag.spv" /o "..\source\v3dEditor\resource\paste_frag_spv.inc"
BinToTxt /i "%OBJ_DIR%\brightPass_frag.spv" /o "..\source\v3dEditor\resource\brightPass_frag_spv.inc"
BinToTxt /i "%OBJ_DIR%\downSampling2x2_frag.spv" /o "..\source\v3dEditor\resource\downSampling2x2_frag_spv.inc"
BinToTxt /i "%OBJ_DIR%\gaussianBlur_frag.spv" /o "..\source\v3dEditor\resource\gaussianBlur_frag_spv.inc"

BinToTxt /i "shaders\mesh.vert" /o "..\source\v3dEditor\resource\\mesh_vert.inc"
BinToTxt /i "shaders\mesh.frag" /o "..\source\v3dEditor\resource\\mesh_frag.inc"

BinToTxt /i "shaders\meshShadow.vert" /o "..\source\v3dEditor\resource\\meshShadow_vert.inc"
BinToTxt /i "shaders\meshShadow.frag" /o "..\source\v3dEditor\resource\\meshShadow_frag.inc"

BinToTxt /i "shaders\meshSelect.vert" /o "..\source\v3dEditor\resource\\meshSelect_vert.inc"
BinToTxt /i "shaders\meshSelect.frag" /o "..\source\v3dEditor\resource\\meshSelect_frag.inc"

BinToTxt /i "images\noise.dds" /o "..\source\v3dEditor\resource\\noise_dds.inc"

pause
