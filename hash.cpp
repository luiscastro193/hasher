#include <emscripten/emscripten.h>
#define CAPI extern "C" EMSCRIPTEN_KEEPALIVE

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <wasm_simd128.h>

constexpr uint64_t PRIME = 0x9E3779B97F4A7C15ULL;
constexpr uint64_t SEED = 0xCBF29CE484222325ULL;

constexpr int LANES = 8;
constexpr size_t WIDTH = sizeof(v128_t);
constexpr size_t BLOCK_SIZE = LANES * WIDTH;

const v128_t PRIME_VEC = wasm_i64x2_const_splat(PRIME);

struct HashState {
	v128_t accumulator[LANES];
	uint8_t remaining[BLOCK_SIZE];
	size_t remaining_n = 0;
	
	HashState() {
		for (int i = 0; i < LANES; i++)
			accumulator[i] = wasm_i64x2_make(SEED ^ PRIME * (2 * i + 1), SEED ^ PRIME * (2 * i + 2));
	}
	
	void absorb(const uint8_t* queue) {
		for (int i = 0; i < LANES; i++)
			accumulator[i] = wasm_i64x2_mul(
				wasm_v128_xor(accumulator[i], wasm_v128_load(queue + i * WIDTH)),
				PRIME_VEC
			);
	}
};

CAPI HashState* create() {
	return new HashState();
}

CAPI uint8_t* allocate(HashState* state, const size_t n) {
	uint8_t* buffer = (uint8_t*)malloc(n + state->remaining_n);
	return buffer + state->remaining_n;
}

CAPI void update(HashState* state, uint8_t* buffer, const size_t n) {
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
	memset(state->remaining + state->remaining_n, 0, BLOCK_SIZE - state->remaining_n);
	state->absorb(state->remaining);
	uint64_t lanes[2 * LANES];
	uint64_t hash = SEED ^ state->remaining_n;
	
	for (int i = 0; i < LANES; i++)
		wasm_v128_store(lanes + 2 * i, state->accumulator[i]);
	
	for (int i = 0; i < 2 * LANES; i++)
		hash = (hash ^ lanes[i]) * PRIME;
	
	delete state;
	return hash;
}
