import module from './hash.js';
const hasherPromise = module();

export default async function hash(stream) {
	let hasher, state, result;
	
	await stream.pipeTo(new WritableStream({
		start: async () => {
			hasher = await hasherPromise;
			state = hasher._create();
		},
		write: chunk => {
			const buffer = hasher._allocate(state, chunk.length);
			hasher.HEAPU8.set(chunk, buffer);
			hasher._update(state, buffer, chunk.length);
		},
		close: () => {result = hasher._digest(state)},
		abort: () => {hasher._digest(state)}
	}, {highWaterMark: 2}));
	
	return result;
}
