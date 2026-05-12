import module from './hash.js';
const hasherPromise = module();

export default async function hash(stream) {
	const hasher = await hasherPromise;
	const state = hasher._create();
	
	await stream.pipeTo(new WritableStream({write: chunk => {
		const buffer = hasher._allocate(state, chunk.length);
		hasher.HEAPU8.set(chunk, buffer);
		hasher._update(state, buffer, chunk.length);
	}})).catch(error => {
		hasher._digest(state);
		throw error;
	});
	
	return hasher._digest(state);
}
