struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLDPOSITION;
    float2 decalUV : TEXCOORD;
};

// カメラ情報 (通常のオブジェクトと同じものを使用)
struct ViewProjectionMatrix
{
    float4x4 view;
    float4x4 projection;
    float4x4 orthoGraphic;
    float3 cameraPosition;
};

// デカール設定
struct DecalSettings
{
    float4x4 decalMatrix; // ワールド空間からデカール空間への変換行列 (通常はデカールワールド行列の逆行列)
    float4 decalColor; // デカールの色
    float decalStrength; // デカールの適用強度
    int enable;
};