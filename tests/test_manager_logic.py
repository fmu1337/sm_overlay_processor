#!/usr/bin/env python3
"""Pure-Python mirror of claim/stack winner selection."""

from __future__ import annotations

from dataclasses import dataclass, field


@dataclass
class Claim:
    token: int
    priority: int
    sequence: int
    path: str


@dataclass
class ClientState:
    claims: list = field(default_factory=list)
    stack: list = field(default_factory=list)
    seq: int = 0
    next_token: int = 1

    def set(self, path, priority):
        token = self.next_token
        self.next_token += 1
        self.seq += 1
        self.claims.append(Claim(token, priority, self.seq, path))
        return token

    def clear(self, token):
        self.claims = [c for c in self.claims if c.token != token]

    def push(self, path):
        self.stack.append(path)

    def pop(self):
        self.stack.pop()

    def active(self):
        if self.stack:
            return self.stack[-1]
        if not self.claims:
            return None
        best = self.claims[0]
        for c in self.claims[1:]:
            if c.priority > best.priority or (
                c.priority == best.priority and c.sequence > best.sequence
            ):
                best = c
        return best.path


def main():
    s = ClientState()
    low = s.set("low", 10)
    high = s.set("high", 100)
    assert s.active() == "high"
    s.clear(high)
    assert s.active() == "low"
    s.push("push")
    assert s.active() == "push"
    s.pop()
    assert s.active() == "low"
    s.set("newer_same", 10)
    assert s.active() == "newer_same"
    s.clear(low)
    assert s.active() == "newer_same"
    print("OK: manager logic")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
