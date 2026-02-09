#ifndef TSUMI_FULLSCREEN_HLSLI
#define TSUMI_FULLSCREEN_HLSLI

// =======================================
// Fullscreen Triangle Vertex Shader Helper
// =======================================
//
// SV_VertexID を使って
// 頂点バッファ不要のフルスクリーントライアングルを生成する
//
// 頂点ID : 0,1,2
//
//  2
//  |＼
//  |  ＼
//  |____＼
//  0      1
//

struct FullscreenVSOut
{
    float4 positionCS : SV_POSITION;
    float2 uv : TEXCOORD0;
};

inline FullscreenVSOut FullscreenVS(uint vertexID)
{
    FullscreenVSOut o;

    // クリップ空間座標（-1〜1）
    // この並びで 1 枚の巨大三角形になる
    float2 pos;
    pos.x = (vertexID == 2) ? 3.0f : -1.0f;
    pos.y = (vertexID == 1) ? 3.0f : -1.0f;

    o.positionCS = float4(pos, 0.0f, 1.0f);

    // UV（0〜1）
    // pos(-1,-1)->uv(0,0)
    // pos( 3,-1)->uv(2,0)
    // pos(-1, 3)->uv(0,2)
    o.uv = pos * 0.5f + 0.5f;

    return o;
}

#endif // TSUMI_FULLSCREEN_HLSLI
