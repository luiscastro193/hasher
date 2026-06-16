#include <emscripten/emscripten.h>
#define CAPI extern "C" EMSCRIPTEN_KEEPALIVE
#define WASM_SIMD_COMPAT_SLOW

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <memory>

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
	uint8_t remaining[BLOCK_SIZE];
	size_t remaining_n = 0;
	
	HashState() {
		for (int i = 0; i < LANES; i++)
			accumulator[i] = v128_t{GOLDEN * (4 * i + 1), GOLDEN * (4 * i + 2), GOLDEN * (4 * i + 3), GOLDEN * (4 * i + 4)};
	}
	
	void absorb(const uint8_t* queue) {
		constexpr v128_t GOLDEN_VEC = {GOLDEN, GOLDEN, GOLDEN, GOLDEN};
		
		for (int i = 0; i < LANES; i++) {
			const v128_u block = *(const v128_u*)(queue + i * WIDTH);
			v128_64 acc = (v128_64)((accumulator[i] ^ block) * GOLDEN_VEC);
			accumulator[i] = (v128_t)(acc ^ acc >> 16 ^ acc << 32) * GOLDEN_VEC;
		}
	}
};

CAPI HashState* create() {
	return new HashState();
}

CAPI uint8_t* allocate(HashState* state, const size_t n) {
	uint8_t* buffer = (uint8_t*)malloc(n + state->remaining_n);
	return buffer + state->remaining_n;
}

CAPI void update(HashState* state, uint8_t* __restrict buffer, const size_t n) {
	buffer -= state->remaining_n;
	memcpy(buffer, state->remaining, state->remaining_n);
	size_t offset, available = n + state->remaining_n;
	
	for (offset = 0; available - offset >= BLOCK_SIZE; offset += BLOCK_SIZE)
		state->absorb(buffer + offset);
	
	state->remaining_n = available - offset;
	memcpy(state->remaining, buffer + offset, state->remaining_n);
	free(buffer);
}

CAPI uint64_t digest(HashState* state) {
	std::unique_ptr<HashState> guard(state);
	memset(state->remaining + state->remaining_n, 0, BLOCK_SIZE - state->remaining_n);
	state->absorb(state->remaining);
	uint64_t hash = state->remaining_n * GOLDEN64;
	
	for (int i = 0; i < LANES; i++) {
		const v128_64 acc = (v128_64)state->accumulator[i];
		hash ^= acc[0];
		hash = (hash ^ hash >> 32) * GOLDEN64;
		hash ^= acc[1];
		hash = (hash ^ hash >> 32) * GOLDEN64;
	}
	
	hash = (hash ^ hash >> 32) * GOLDEN64;
	return hash ^ hash >> 32;
}
