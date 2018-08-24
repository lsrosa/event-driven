import re
import numpy as np
from tqdm import tqdm
import os

pattern = re.compile('(\d+) (\d+\.\d+) AE \((.*)\)')


def pretty_print_AE(bottlenumber, timestamp, events):
    return '%d %f AE (%s)' %(bottlenumber, timestamp,
                             ' '.join(map(str, list(events)))
)

def do_collapse(filename):
    with open(filename, 'r') as f:
        content = f.read()
        found = pattern.findall(content)
        buffer = []
        first_timestamp = None
        for elem in tqdm(found):

            events = np.int32(elem[2].split())
            bottlenumber = np.int32(elem[0])
            timestamp = np.float64(elem[1])
            #print(pretty_print_AE(bottlenumber, timestamp, events))
            buffer.append(events)
            if first_timestamp is None:
                first_timestamp = timestamp
                
            if (timestamp-first_timestamp > 2e-3):
                first_timestamp = timestamp
                pp = pretty_print_AE(bottlenumber, timestamp,
                                      np.concatenate(buffer))
                buffer = []
                yield pp

if __name__ == '__main__':
    for i in range(1,8):
        infile = '/home/miacono/datasets/saccade_attention/{}/events/data.log'.format(i)
        outfile = '/home/miacono/datasets/saccade_attention/{}/events/data_collapsed.log'.format(i)
        text_out = '\n'.join([line for line in do_collapse(infile)])
        with open(outfile, 'w') as of:
            of.write(text_out)
        os.rename(infile, os.path.join(os.path.dirname(infile), 'data_original.log'))
        os.rename(outfile, os.path.join(os.path.dirname(outfile), 'data.log'))
       
