// SMHasher3 port
#include "Platform.h"
#include "Hashlib.h"

constexpr uint32_t GOLDEN = 0x9E3779B9U;
constexpr uint64_t GOLDEN64 = 0x9E3779B97F4A7C15ULL;

typedef uint32_t v128_t __attribute__((vector_size(16)));
typedef uint32_t v128_u __attribute__((vector_size(16), aligned(1), may_alias));
typedef uint64_t v128_64 __attribute__((vector_size(16)));

constexpr int LANES = 8;
constexpr size_t WIDTH = sizeof(v128_t);
constexpr size_t BLOCK_SIZE = LANES * WIDTH;

struct HashState {
	v128_t accumulator[LANES];
	
	HashState() {
		for (int i = 0; i < LANES; i++)
			accumulator[i] = v128_t{GOLDEN * (4 * i + 1), GOLDEN * (4 * i + 2), GOLDEN * (4 * i + 3), GOLDEN * (4 * i + 4)};
	}
	
	void absorb(const uint8_t* queue) {
		constexpr v128_t golden_vec = {GOLDEN, GOLDEN, GOLDEN, GOLDEN};
		
		for (int i = 0; i < LANES; i++) {
			const v128_u block = *(const v128_u*)(queue + i * WIDTH);
			v128_64 acc = (v128_64)((accumulator[i] ^ block) * golden_vec);
			accumulator[i] = (v128_t)(acc ^ acc >> 16 ^ acc << 32) * golden_vec;
		}
	}
	
	uint64_t digest(const uint8_t* tail, const size_t tail_n) {
		uint8_t block[BLOCK_SIZE] = {};
		memcpy(block, tail, tail_n);
		absorb(block);
		uint64_t hash = tail_n * GOLDEN64;
		
		for (int i = 0; i < LANES; i++) {
			const v128_64 acc = (v128_64)accumulator[i];
			hash ^= acc[0];
			hash = (hash ^ hash >> 32) * GOLDEN64;
			hash ^= acc[1];
			hash = (hash ^ hash >> 32) * GOLDEN64;
		}
		
		hash = (hash ^ hash >> 32) * GOLDEN64;
		return hash ^ hash >> 32;
	}
};

static void hasher64(const void* in, const size_t len, const seed_t seed, void* out) {
	const uint8_t* data = (const uint8_t*)in;
	HashState state;
	size_t offset;
	
	for (int i = 0; i < LANES; i++)
		state.accumulator[i] = (v128_t)((v128_64)state.accumulator[i] ^ v128_64{seed, seed});
	
	for (offset = 0; len - offset >= BLOCK_SIZE; offset += BLOCK_SIZE)
		state.absorb(data + offset);
	
	const uint64_t hash = state.digest(data + offset, len - offset);
	memcpy(out, &hash, sizeof(hash));
}

REGISTER_FAMILY(hasher,
	$.src_url = "https://github.com/luiscastro193/hasher",
	$.src_status = HashFamilyInfo::SRC_ACTIVE
);

REGISTER_HASH(hasher64,
	$.desc = "WebAssembly optimized hasher",
	$.hash_flags = 0,
	$.impl_flags = FLAG_IMPL_MULTIPLY | FLAG_IMPL_MULTIPLY_64_64,
	$.bits = 64,
	$.verification_LE = 0x3D95EFE7,
	$.verification_BE = 0x0,
	$.hashfn_native = hasher64,
	$.hashfn_bswap = hasher64
);
