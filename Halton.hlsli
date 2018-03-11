// ---------------------------------------------------------------------
// Code in this file has been derived from the code obtained from 
//                  http://gruenschloss.org/
// The below license accompanied the original code:
// ---------------------------------------------------------------------

// Copyright (c) 2012 Leonhard Gruenschloss (leonhard@gruenschloss.org)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#ifndef HALTON_HLSLI
#define HALTON_HLSLI



// Special case: radical inverse in base 2, with direct bit reversal.
float halton2(uint index)
{
	index = (index << 16) | (index >> 16);
	index = ((index & 0x00ff00ff) << 8) | ((index & 0xff00ff00) >> 8);
	index = ((index & 0x0f0f0f0f) << 4) | ((index & 0xf0f0f0f0) >> 4);
	index = ((index & 0x33333333) << 2) | ((index & 0xcccccccc) >> 2);
	index = ((index & 0x55555555) << 1) | ((index & 0xaaaaaaaa) >> 1);
	uint result = 0x3f800000u | (index >> 9);
	return asfloat(result) - 1.f;
}

float halton3(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 243u, 3, 0)) * 14348907u +
		gHaltonTexture.Load(int3((index / 243u) % 243u, 3, 0)) * 59049u +
		gHaltonTexture.Load(int3((index / 59049u) % 243u, 3, 0)) * 243u +
		gHaltonTexture.Load(int3((index / 14348907u) % 243u, 3, 0))) * float(0.99999988079071044921875 / 3486784401u); // Results in [0,1).
}

float halton5(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 125u, 5, 0)) * 1953125u +
		gHaltonTexture.Load(int3((index / 125u) % 125u, 5, 0)) * 15625u +
		gHaltonTexture.Load(int3((index / 15625u) % 125u, 5, 0)) * 125u +
		gHaltonTexture.Load(int3((index / 1953125u) % 125u, 5, 0))) * float(0.99999988079071044921875 / 244140625u); // Results in [0,1).
}

float halton7(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 343u, 7, 0)) * 117649u +
		gHaltonTexture.Load(int3((index / 343u) % 343u, 7, 0)) * 343u +
		gHaltonTexture.Load(int3((index / 117649u) % 343u, 7, 0))) * float(0.99999988079071044921875 / 40353607u); // Results in [0,1).
}

float halton11(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 121u, 11, 0)) * 1771561u +
		gHaltonTexture.Load(int3((index / 121u) % 121u, 11, 0)) * 14641u +
		gHaltonTexture.Load(int3((index / 14641u) % 121u, 11, 0)) * 121u +
		gHaltonTexture.Load(int3((index / 1771561u) % 121u, 11, 0))) * float(0.99999988079071044921875 / 214358881u); // Results in [0,1).
}

float halton13(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 169u, 13, 0)) * 4826809u +
		gHaltonTexture.Load(int3((index / 169u) % 169u, 13, 0)) * 28561u +
		gHaltonTexture.Load(int3((index / 28561u) % 169u, 13, 0)) * 169u +
		gHaltonTexture.Load(int3((index / 4826809u) % 169u, 13, 0))) * float(0.99999988079071044921875 / 815730721u); // Results in [0,1).
}

float halton17(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 289u, 17, 0)) * 83521u +
		gHaltonTexture.Load(int3((index / 289u) % 289u, 17, 0)) * 289u +
		gHaltonTexture.Load(int3((index / 83521u) % 289u, 17, 0))) * float(0.99999988079071044921875 / 24137569u); // Results in [0,1).
}

float halton19(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 361u, 19, 0)) * 130321u +
		gHaltonTexture.Load(int3((index / 361u) % 361u, 19, 0)) * 361u +
		gHaltonTexture.Load(int3((index / 130321u) % 361u, 19, 0))) * float(0.99999988079071044921875 / 47045881u); // Results in [0,1).
}

float halton23(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 23u, 23, 0)) * 148035889u +
		gHaltonTexture.Load(int3((index / 23u) % 23u, 23, 0)) * 6436343u +
		gHaltonTexture.Load(int3((index / 529u) % 23u, 23, 0)) * 279841u +
		gHaltonTexture.Load(int3((index / 12167u) % 23u, 23, 0)) * 12167u +
		gHaltonTexture.Load(int3((index / 279841u) % 23u, 23, 0)) * 529u +
		gHaltonTexture.Load(int3((index / 6436343u) % 23u, 23, 0)) * 23u +
		gHaltonTexture.Load(int3((index / 148035889u) % 23u, 23, 0))) * float(0.99999988079071044921875 / 3404825447u); // Results in [0,1).
}

float halton29(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 29u, 29, 0)) * 20511149u +
		gHaltonTexture.Load(int3((index / 29u) % 29u, 29, 0)) * 707281u +
		gHaltonTexture.Load(int3((index / 841u) % 29u, 29, 0)) * 24389u +
		gHaltonTexture.Load(int3((index / 24389u) % 29u, 29, 0)) * 841u +
		gHaltonTexture.Load(int3((index / 707281u) % 29u, 29, 0)) * 29u +
		gHaltonTexture.Load(int3((index / 20511149u) % 29u, 29, 0))) * float(0.99999988079071044921875 / 594823321u); // Results in [0,1).
}

float halton31(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 31u, 31, 0)) * 28629151u +
		gHaltonTexture.Load(int3((index / 31u) % 31u, 31, 0)) * 923521u +
		gHaltonTexture.Load(int3((index / 961u) % 31u, 31, 0)) * 29791u +
		gHaltonTexture.Load(int3((index / 29791u) % 31u, 31, 0)) * 961u +
		gHaltonTexture.Load(int3((index / 923521u) % 31u, 31, 0)) * 31u +
		gHaltonTexture.Load(int3((index / 28629151u) % 31u, 31, 0))) * float(0.99999988079071044921875 / 887503681u); // Results in [0,1).
}

float halton37(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 37u, 37, 0)) * 69343957u +
		gHaltonTexture.Load(int3((index / 37u) % 37u, 37, 0)) * 1874161u +
		gHaltonTexture.Load(int3((index / 1369u) % 37u, 37, 0)) * 50653u +
		gHaltonTexture.Load(int3((index / 50653u) % 37u, 37, 0)) * 1369u +
		gHaltonTexture.Load(int3((index / 1874161u) % 37u, 37, 0)) * 37u +
		gHaltonTexture.Load(int3((index / 69343957u) % 37u, 37, 0))) * float(0.99999988079071044921875 / 2565726409u); // Results in [0,1).
}

float halton41(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 41u, 41, 0)) * 2825761u +
		gHaltonTexture.Load(int3((index / 41u) % 41u, 41, 0)) * 68921u +
		gHaltonTexture.Load(int3((index / 1681u) % 41u, 41, 0)) * 1681u +
		gHaltonTexture.Load(int3((index / 68921u) % 41u, 41, 0)) * 41u +
		gHaltonTexture.Load(int3((index / 2825761u) % 41u, 41, 0))) * float(0.99999988079071044921875 / 115856201u); // Results in [0,1).
}

float halton43(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 43u, 43, 0)) * 3418801u +
		gHaltonTexture.Load(int3((index / 43u) % 43u, 43, 0)) * 79507u +
		gHaltonTexture.Load(int3((index / 1849u) % 43u, 43, 0)) * 1849u +
		gHaltonTexture.Load(int3((index / 79507u) % 43u, 43, 0)) * 43u +
		gHaltonTexture.Load(int3((index / 3418801u) % 43u, 43, 0))) * float(0.99999988079071044921875 / 147008443u); // Results in [0,1).
}

float halton47(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 47u, 47, 0)) * 4879681u +
		gHaltonTexture.Load(int3((index / 47u) % 47u, 47, 0)) * 103823u +
		gHaltonTexture.Load(int3((index / 2209u) % 47u, 47, 0)) * 2209u +
		gHaltonTexture.Load(int3((index / 103823u) % 47u, 47, 0)) * 47u +
		gHaltonTexture.Load(int3((index / 4879681u) % 47u, 47, 0))) * float(0.99999988079071044921875 / 229345007u); // Results in [0,1).
}

float halton53(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 53u, 53, 0)) * 7890481u +
		gHaltonTexture.Load(int3((index / 53u) % 53u, 53, 0)) * 148877u +
		gHaltonTexture.Load(int3((index / 2809u) % 53u, 53, 0)) * 2809u +
		gHaltonTexture.Load(int3((index / 148877u) % 53u, 53, 0)) * 53u +
		gHaltonTexture.Load(int3((index / 7890481u) % 53u, 53, 0))) * float(0.99999988079071044921875 / 418195493u); // Results in [0,1).
}

float halton59(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 59u, 59, 0)) * 12117361u +
		gHaltonTexture.Load(int3((index / 59u) % 59u, 59, 0)) * 205379u +
		gHaltonTexture.Load(int3((index / 3481u) % 59u, 59, 0)) * 3481u +
		gHaltonTexture.Load(int3((index / 205379u) % 59u, 59, 0)) * 59u +
		gHaltonTexture.Load(int3((index / 12117361u) % 59u, 59, 0))) * float(0.99999988079071044921875 / 714924299u); // Results in [0,1).
}

float halton61(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 61u, 61, 0)) * 13845841u +
		gHaltonTexture.Load(int3((index / 61u) % 61u, 61, 0)) * 226981u +
		gHaltonTexture.Load(int3((index / 3721u) % 61u, 61, 0)) * 3721u +
		gHaltonTexture.Load(int3((index / 226981u) % 61u, 61, 0)) * 61u +
		gHaltonTexture.Load(int3((index / 13845841u) % 61u, 61, 0))) * float(0.99999988079071044921875 / 844596301u); // Results in [0,1).
}

float halton67(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 67u, 67, 0)) * 20151121u +
		gHaltonTexture.Load(int3((index / 67u) % 67u, 67, 0)) * 300763u +
		gHaltonTexture.Load(int3((index / 4489u) % 67u, 67, 0)) * 4489u +
		gHaltonTexture.Load(int3((index / 300763u) % 67u, 67, 0)) * 67u +
		gHaltonTexture.Load(int3((index / 20151121u) % 67u, 67, 0))) * float(0.99999988079071044921875 / 1350125107u); // Results in [0,1).
}

float halton71(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 71u, 71, 0)) * 25411681u +
		gHaltonTexture.Load(int3((index / 71u) % 71u, 71, 0)) * 357911u +
		gHaltonTexture.Load(int3((index / 5041u) % 71u, 71, 0)) * 5041u +
		gHaltonTexture.Load(int3((index / 357911u) % 71u, 71, 0)) * 71u +
		gHaltonTexture.Load(int3((index / 25411681u) % 71u, 71, 0))) * float(0.99999988079071044921875 / 1804229351u); // Results in [0,1).
}

float halton73(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 73u, 73, 0)) * 28398241u +
		gHaltonTexture.Load(int3((index / 73u) % 73u, 73, 0)) * 389017u +
		gHaltonTexture.Load(int3((index / 5329u) % 73u, 73, 0)) * 5329u +
		gHaltonTexture.Load(int3((index / 389017u) % 73u, 73, 0)) * 73u +
		gHaltonTexture.Load(int3((index / 28398241u) % 73u, 73, 0))) * float(0.99999988079071044921875 / 2073071593u); // Results in [0,1).
}

float halton79(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 79u, 79, 0)) * 38950081u +
		gHaltonTexture.Load(int3((index / 79u) % 79u, 79, 0)) * 493039u +
		gHaltonTexture.Load(int3((index / 6241u) % 79u, 79, 0)) * 6241u +
		gHaltonTexture.Load(int3((index / 493039u) % 79u, 79, 0)) * 79u +
		gHaltonTexture.Load(int3((index / 38950081u) % 79u, 79, 0))) * float(0.99999988079071044921875 / 3077056399u); // Results in [0,1).
}

float halton83(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 83u, 83, 0)) * 47458321u +
		gHaltonTexture.Load(int3((index / 83u) % 83u, 83, 0)) * 571787u +
		gHaltonTexture.Load(int3((index / 6889u) % 83u, 83, 0)) * 6889u +
		gHaltonTexture.Load(int3((index / 571787u) % 83u, 83, 0)) * 83u +
		gHaltonTexture.Load(int3((index / 47458321u) % 83u, 83, 0))) * float(0.99999988079071044921875 / 3939040643u); // Results in [0,1).
}

float halton89(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 89u, 89, 0)) * 704969u +
		gHaltonTexture.Load(int3((index / 89u) % 89u, 89, 0)) * 7921u +
		gHaltonTexture.Load(int3((index / 7921u) % 89u, 89, 0)) * 89u +
		gHaltonTexture.Load(int3((index / 704969u) % 89u, 89, 0))) * float(0.99999988079071044921875 / 62742241u); // Results in [0,1).
}

float halton97(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 97u, 97, 0)) * 912673u +
		gHaltonTexture.Load(int3((index / 97u) % 97u, 97, 0)) * 9409u +
		gHaltonTexture.Load(int3((index / 9409u) % 97u, 97, 0)) * 97u +
		gHaltonTexture.Load(int3((index / 912673u) % 97u, 97, 0))) * float(0.99999988079071044921875 / 88529281u); // Results in [0,1).
}

float halton101(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 101u, 101, 0)) * 1030301u +
		gHaltonTexture.Load(int3((index / 101u) % 101u, 101, 0)) * 10201u +
		gHaltonTexture.Load(int3((index / 10201u) % 101u, 101, 0)) * 101u +
		gHaltonTexture.Load(int3((index / 1030301u) % 101u, 101, 0))) * float(0.99999988079071044921875 / 104060401u); // Results in [0,1).
}

float halton103(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 103u, 103, 0)) * 1092727u +
		gHaltonTexture.Load(int3((index / 103u) % 103u, 103, 0)) * 10609u +
		gHaltonTexture.Load(int3((index / 10609u) % 103u, 103, 0)) * 103u +
		gHaltonTexture.Load(int3((index / 1092727u) % 103u, 103, 0))) * float(0.99999988079071044921875 / 112550881u); // Results in [0,1).
}

float halton107(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 107u, 107, 0)) * 1225043u +
		gHaltonTexture.Load(int3((index / 107u) % 107u, 107, 0)) * 11449u +
		gHaltonTexture.Load(int3((index / 11449u) % 107u, 107, 0)) * 107u +
		gHaltonTexture.Load(int3((index / 1225043u) % 107u, 107, 0))) * float(0.99999988079071044921875 / 131079601u); // Results in [0,1).
}

float halton109(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 109u, 109, 0)) * 1295029u +
		gHaltonTexture.Load(int3((index / 109u) % 109u, 109, 0)) * 11881u +
		gHaltonTexture.Load(int3((index / 11881u) % 109u, 109, 0)) * 109u +
		gHaltonTexture.Load(int3((index / 1295029u) % 109u, 109, 0))) * float(0.99999988079071044921875 / 141158161u); // Results in [0,1).
}

float halton113(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 113u, 113, 0)) * 1442897u +
		gHaltonTexture.Load(int3((index / 113u) % 113u, 113, 0)) * 12769u +
		gHaltonTexture.Load(int3((index / 12769u) % 113u, 113, 0)) * 113u +
		gHaltonTexture.Load(int3((index / 1442897u) % 113u, 113, 0))) * float(0.99999988079071044921875 / 163047361u); // Results in [0,1).
}

float halton127(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 127u, 127, 0)) * 2048383u +
		gHaltonTexture.Load(int3((index / 127u) % 127u, 127, 0)) * 16129u +
		gHaltonTexture.Load(int3((index / 16129u) % 127u, 127, 0)) * 127u +
		gHaltonTexture.Load(int3((index / 2048383u) % 127u, 127, 0))) * float(0.99999988079071044921875 / 260144641u); // Results in [0,1).
}

float halton131(uint index)
{
	return (gHaltonTexture.Load(int3((index) % 131u, 131, 0)) * 2248091u +
		gHaltonTexture.Load(int3((index / 131u) % 131u, 131, 0)) * 17161u +
		gHaltonTexture.Load(int3((index / 17161u) % 131u, 131, 0)) * 131u +
		gHaltonTexture.Load(int3((index / 2248091u) % 131u, 131, 0))) * float(0.99999988079071044921875 / 294499921u); // Results in [0,1).
}

float haltonSampler(uint dimension, uint index)
{
	switch (dimension)
	{
	case 0:  return halton2(index);
	case 1:  return halton3(index);
	case 2:  return halton5(index);
	case 3:  return halton7(index);
	case 4:  return halton11(index);
	case 5:  return halton13(index);
	case 6:  return halton17(index);
	case 7:  return halton19(index);
	case 8:  return halton23(index);
	case 9:  return halton29(index);
	case 10: return halton31(index);
	case 11: return halton37(index);
	case 12: return halton41(index);
	case 13: return halton43(index);
	case 14: return halton47(index);
	case 15: return halton53(index);
	case 16: return halton59(index);
	case 17: return halton61(index);
	case 18: return halton67(index);
	case 19: return halton71(index);
	case 20: return halton73(index);
	case 21: return halton79(index);
	case 22: return halton83(index);
	case 23: return halton89(index);
	case 24: return halton97(index);
	case 25: return halton101(index);
	case 26: return halton103(index);
	case 27: return halton107(index);
	case 28: return halton109(index);
	case 29: return halton113(index);
	case 30: return halton127(index);
	case 31: return halton131(index);
	}
	return 0.f;
}

float radicalInverse(uint sampIdx, uint base)
{
	float H = 0;

	// This parses through sampIdx, in it's representation in the specified
	//     base (e.g., base 2) one digit at a time, reflecting each individual
	//     digit around the decimal point.
	const float oneOverBase = 1.0f / float(base);
	for (float mult = oneOverBase;    // Use our precomputed 1/base
		sampIdx != 0;
		sampIdx /= base,         // Do *integer* division by our base
		mult *= oneOverBase)         // Do floating-point division by base
									 //   (using mult by precomputed 1/base)
	{
		// Accumulate reflection of current digit around decimal point
		//     The mod operation takes the least significant digit.
		//     mult reflects this digit around the decimal (changes per digit)
		H += float(sampIdx % base) * mult;
	}
	return H;
}

float naiveHaltonSampler(uint dimension, uint index)
{
	int base = 0;
	switch (dimension)
	{
	case 0:  base = 2;   break;
	case 1:  base = 3;   break;
	case 2:  base = 5;   break;
	case 3:  base = 7;   break;
	case 4:  base = 11;  break;
	case 5:  base = 13;  break;
	case 6:  base = 17;  break;
	case 7:  base = 19;  break;
	case 8:  base = 23;  break;
	case 9:  base = 29;  break;
	case 10: base = 31;  break;
	case 11: base = 37;  break;
	case 12: base = 41;  break;
	case 13: base = 43;  break;
	case 14: base = 47;  break;
	case 15: base = 53;  break;
	case 16: base = 59;  break;
	case 17: base = 61;  break;
	case 18: base = 67;  break;
	case 19: base = 71;  break;
	case 20: base = 73;  break;
	case 21: base = 79;  break;
	case 22: base = 83;  break;
	case 23: base = 89;  break;
	case 24: base = 97;  break;
	case 25: base = 101; break;
	case 26: base = 103; break;
	case 27: base = 107; break;
	case 28: base = 109; break;
	case 29: base = 113; break;
	case 30: base = 127; break;
	case 31: base = 131; break;
	default: base = 2;   break;
	}
	return radicalInverse(index, base);
}

uint halton2_inverse(uint index, const uint digits)
{
	index = (index << 16) | (index >> 16);
	index = ((index & 0x00ff00ff) << 8) | ((index & 0xff00ff00) >> 8);
	index = ((index & 0x0f0f0f0f) << 4) | ((index & 0xf0f0f0f0) >> 4);
	index = ((index & 0x33333333) << 2) | ((index & 0xcccccccc) >> 2);
	index = ((index & 0x55555555) << 1) | ((index & 0xaaaaaaaa) >> 1);
	return index >> (32 - digits);
}

uint halton3_inverse(uint index, const uint digits)
{
	uint result = 0;
	for (uint d = 0; d < digits; ++d)
	{
		result = result * 3 + index % 3;
		index /= 3;
	}
	return result;
}

uint halton_get_index(uint x, uint y, const uint i)
{
	x %= 256;
	y %= 256;
	uint m_x = 76545;
	uint m_y = 110080;
	uint m_increment = 186624;
	uint m_p2 = 8;
	uint m_p3 = 6;

	// Promote to 64 bits to avoid overflow.
	const uint /*long long*/ hx = halton2_inverse(x, m_p2);
	const uint /*long long*/ hy = halton3_inverse(y, m_p3);
	// Apply Chinese remainder theorem.
	const uint offset = ((hx * m_x + hy * m_y) % m_increment);
	return offset + i * m_increment;
}

struct HaltonState
{
	uint dimension;
	uint sequenceIdx;
};

void setupHalton(inout HaltonState hState, int x, int y, int path, int numpaths, int frameId, int loopLength)
{
	const uint len = loopLength * numpaths;
	const uint sampleIdx = (frameId * numpaths + path) % len;

	hState.dimension = 2; // first two dimensions are for sampling the subpixel
	hState.sequenceIdx = halton_get_index(x, y, sampleIdx);
}

float nextHalton(inout HaltonState hState)
{
	return haltonSampler(hState.dimension++, hState.sequenceIdx);
}

#endif