"""No hardware: verify actual command stream, RAM addressing and failure exits."""
from pathlib import Path
import subprocess

binary = Path(__file__).resolve().parents[1] / 'build/oled-lab'

def run(*args):
    return subprocess.run([str(binary), *args], capture_output=True,
                          text=True, timeout=10)

result = run('--trace', 'pages')
assert result.returncode == 0, result.stderr
tx = [bytes.fromhex(line.split(':', 1)[1])
      for line in result.stdout.splitlines() if line.startswith('TX ')]
assert tx[0][0:2] == b'\x00\xae'
assert tx[1] == bytes([0,0x21,0,127,0x22,0,3])
assert tx[34] == b'\x00\xaf'
assert tx[35] == tx[1]
assert len(tx[36:]) == 32
assert all(len(packet)==17 and packet[0]==0x40 for packet in tx[36:])
frame = b''.join(packet[1:] for packet in tx[36:])
assert frame == b'\x01'*128 + b'\x02'*128 + b'\x04'*128 + b'\x08'*128
for index in (1, 3, 34, 35, 40):
    result = run('--trace', '--fail-at', str(index), 'pages')
    assert result.returncode == 1
    assert 'OLED operation' in result.stderr
for args in [[], ['bad'], ['--trace','--bus','/dev/null','demo'],
             ['--fail-at','2','demo'], ['--trace','--fail-at','-1','demo']]:
    assert run(*args).returncode == 2, args
assert run('--bus','/nonexistent/oled-adapter','init').returncode == 1
assert run('--trace','demo').returncode == 0
print('PASS CLI: wire framing, page data, injected I/O failures, invalid arguments')
