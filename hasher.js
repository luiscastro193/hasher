import module from './hash.js';
const hasherPromise = module();

export default async function hash(stream) {
	const hasher = await hasherPromise;
	const state = hasher._create();
	let remaining = 0;
	
	await stream.pipeTo(new WritableStream({write: chunk => {
		const buffer = hasher._malloc(chunk.length + remaining);
		hasher.HEAPU8.set(chunk, buffer + remaining);
		remaining = hasher._update(state, buffer, chunk.length);
	}})).catch(error => {
		hasher._digest(state);
		throw error;
	});
	
	return hasher._digest(state);
}
