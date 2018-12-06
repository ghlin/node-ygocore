import { OCGEngine } from 'ygocore-interface';

const raw = require('../build/Release/ocgcore');

function engineSetResponse(duel: number, response: Buffer) {
  raw.setResponse(duel, response.buffer.slice(
    response.byteOffset, response.byteOffset + response.byteLength
  ));
}

export const engine = { ...raw, setResponse: engineSetResponse } as OCGEngine<number>;
