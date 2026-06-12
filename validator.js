"use strict";
import hash from './hasher.js';

const input = document.querySelector('textarea');
const result = document.querySelector('p');

input.oninput = async () => {
	const text = input.value.trim();
	
	if (text) {
		result.textContent = await hash(new Blob([text]).stream()).then(h => 
			BigInt.asUintN(64, h).toString(36)
		).catch(e => {
			console.error(e);
			return "Error";
		});
	}
	else
		result.textContent = '';
}

if (input.value) input.oninput();
