struct VSOut
{
	float4 col : Color;
	float4 pos : SV_Position;
};

VSOut main(float4 col : Color, float2 pos : Position)
{
	VSOut vsout;
	vsout.pos = float4(pos.x, pos.y, 0.0f, 1.0f);
	vsout.col = col;
	return vsout;
}