#include <emscripten/emscripten.h>
#define CAPI extern "C" EMSCRIPTEN_KEEPALIVE
#define WASM_SIMD_COMPAT_SLOW

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <memory>

constexpr uint32_t SEED = 0x811C9DC5U;
constexpr uint32_t PRIME = 0x9E3779B9U;
constexpr uint64_t PRIME64 = 0x9E3779B97F4A7C15ULL;

typedef uint32_t v128_t __attribute__((vector_size(16)));
typedef uint32_t v128_u __attribute__((vector_size(16), aligned(1), may_alias));
typedef uint64_t v128_64 __attribute__((vector_size(16)));

constexpr int LANES = 16;
constexpr size_t WIDTH = sizeof(v128_t);
constexpr size_t BLOCK_SIZE = LANES * WIDTH;

struct HashState {
	v128_t accumulator[LANES];
	uint8_t remaining[BLOCK_SIZE];
	size_t remaining_n = 0;
	
	HashState() {
		for (int i = 0; i < LANES; i++)
			accumulator[i] = v128_t{SEED ^ PRIME * 4 * i, SEED ^ PRIME * (4 * i + 1), SEED ^ PRIME * (4 * i + 2), SEED ^ PRIME * (4 * i + 3)};
	}
	
	void absorb(const uint8_t* queue) {
		for (int i = 0; i < LANES; i++)
			accumulator[i] = (accumulator[i] ^ *(const v128_u*)(queue + i * WIDTH)) * v128_t{PRIME, PRIME, PRIME, PRIME};
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
	v128_64 joined = (v128_64)state->accumulator[0];
	
	for (int i = 1; i < LANES; i++)
		joined ^= (v128_64)state->accumulator[i];
	
	uint64_t hash = state->remaining_n;
	hash = (hash ^ joined[0]) * PRIME64;
	hash = (hash ^ joined[1]) * PRIME64;
	return hash;
}
