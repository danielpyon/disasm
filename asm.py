#!/usr/bin/python3

import json
import functools
import time

def timer(func):
    @functools.wraps(func)
    def wrapper_timer(*args, **kwargs):
        start_time = time.perf_counter()
        value = func(*args, **kwargs)
        end_time = time.perf_counter()
        run_time = end_time - start_time
        print(f"Finished {func.__name__!r} in {run_time:.4f} secs")
        return value
    return wrapper_timer

def load_json(fname):
    with open(fname, 'r') as f:
        contents = json.loads(f.read())
    return contents

def write_json(fname, contents):
    with open(fname, 'w') as f:
        f.write(contents)

def serve_asm():
    while True:
        op = input()
        if op not in x86:
            print('invalid opcode')
        else:
            print(x86[op])

def fmt_x86():
    x86 = load_json('x86_64.json')

    instructions = x86['instructions']

    opcode_to_encodings = dict()

    for k, v in instructions.items():
        forms = v['forms']

        for form in forms:
            for enc in form['encodings']:
                enc['name'] = k
                opcode = enc['opcode']['byte']
                if opcode not in opcode_to_encodings:
                    opcode_to_encodings[opcode] = list()
                opcode_to_encodings[opcode].append(enc)

    write_json('x86_64_fmt.json', json.dumps(opcode_to_encodings))

# 1) invert json: opcode -> encodings + instruction name
#        - save to file
# 2) register decoding

@timer
def find(opcode):
    return x86[opcode]

x86 = load_json('x86_64_fmt.json')
serve_asm()


