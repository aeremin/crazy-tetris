#ifndef SAMPLERS_FX
#define SAMPLERS_FX

SamplerState gAnisotropicSamWrap
{
    Filter = ANISOTROPIC;
    AddressU = WRAP;
    AddressV = WRAP;

};

SamplerState gAnisotropicSamBorder
{
    Filter = ANISOTROPIC;
    AddressU = BORDER;
    AddressV = BORDER;
    BorderColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
};


SamplerState gShadowSam
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};


#endif
