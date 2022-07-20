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

def load():
    with open('x86_64.json') as f:
        x86 = json.loads(f.read())
    return x86

def serve_asm():
    while True:
        op = input()
        if op not in opcode_to_encodings:
            print('invalid opcode')
        else:
            print(opcode_to_encodings[op])

def xform_json():
    with open('x86_64.json', 'r') as f:
        x86 = json.loads(f.read())
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

    with open('x86_64_fmt.json', 'w') as f:
        f.write(json.dumps(opcode_to_encodings))

# 1) invert json: opcode -> encodings + instruction name
#        - save to file
# 2) register decoding

@timer
def find(opcode):
    print(x86['instructions']['MOV']['forms'][0]['encodings'][0])

if __name__ == '__main__':
    # x86 = load()
    # find(0x8d)

    xform_json()
