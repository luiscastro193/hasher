"use strict";
import hash from './hasher.js';

const input = document.querySelector('input');
const result = document.querySelector('p');

input.onchange = async () => {
	input.disabled = true;
	result.textContent = "Loading...";
	
	result.textContent = await hash(input.files[0].stream()).catch(e => {
		console.error(e);
		return "Error";
	});
	
	input.disabled = false;
	input.focus();
}

if (input.value) input.onchange();
