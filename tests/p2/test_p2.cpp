// tests/p2/test_p2.cpp
//
// YOUR test suite goes here. At least 12 assert-based test cases — see
// spec §5 for the required categories and the sample test for the
// expected level of rigor.
//
// This file is a stub so the project builds out of the box; replace the
// body of main() with your own tests.

#include "core/conversation.h"
#include "core/message.h"
#include "core/sentinel_scanner.h"
#include "harness/harness.h"
#include "model/replay_client.h"
#include "model/scripted_client.h"

#include <cassert>
#include <string>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <cstdio>
#include <utility>

class TestInput : public InputSource {
public:
    TestInput(std::string a, std::string b = "")
        : first(a), second(b) {
    }

    std::string read_line() override {
        if (i == 0) {
            ++i;
            return first;
        }

        if (i == 1 && !second.empty()) {
            ++i;
            return second;
        }

        eof = true;
        return "";
    }

    bool is_eof() const override {
        return eof;
    }

private:
    std::string first;
    std::string second;
    int i = 0;
    bool eof = false;
};

class TestOutput : public OutputSink {
public:
    void write(std::string_view) override {}
};

int main() {

    //test 1: empty conversation bounds

    {
        Conversation test;
        assert(test.size() == 0);
        assert(test.begin() == test.end());

        try {
            test.at(0);
            assert(false);
        }
        catch (const std::out_of_range&) {}
    }

    //Test 2: System message must remain first in the conversation

    {
        Conversation test;
        test.append(Message(Role::System, "system"));
        test.append(Message(Role::User, "hello"));

        assert(test.at(0).role() == Role::System);
        assert(test.at(1).role() == Role::User);
    }

    //Test 3: Rule of five copy test

    {
        Conversation test;
        test.append(Message(Role::User, "something here"));

        Conversation dup(test);

        assert(dup.size() == 1);
        assert(test.begin() != dup.begin());
        assert(dup.at(0).content() == "something here");
    }

    //Test 4: Rule of five move test
    {
        Conversation test;
        test.append(Message(Role::User, "something here"));

        const Message* ptr = test.begin();
        
        Conversation mov(std::move(test));

        assert(mov.begin() == ptr);
        assert(test.begin() == nullptr);
        assert(test.size() == 0);


    }

    //Test 5: doubling growth preserves stored messages

    {
        Conversation test;

        test.append(Message(Role::User, "0"));
        const Message* p1 = test.begin();

        test.append(Message(Role::User, "1"));
        const Message* p2 = test.begin();

        test.append(Message(Role::User, "2"));
        const Message* p3 = test.begin();

        test.append(Message(Role::User, "3"));

        assert(p1 != p2);
        assert(p2 != p3);
        assert(test.begin() == p3);
        assert(test.size() == 4);
        assert(test.at(3).content() == "3");
    }

    //Test 6: If no sentinel then text must not change

    {
        SentinelScanner test("<END>");
        auto st1 = test.feed("hello");
        auto st2 = test.flush();

        assert(!st1.sentinel_found);
        assert(st1.safe_text + st2.safe_text == "hello");
    }


    //Test 7: Sentinal is detected everywhere

    {
        std::string sentinel = "<|end_conversation|>";
        std::string text = "bye" + sentinel;

        for (size_t i = 0; i <= text.size(); ++i) {
            SentinelScanner s(sentinel);
            auto a = s.feed(text.substr(0, i));
            auto b = s.feed(text.substr(i));
            assert(a.sentinel_found || b.sentinel_found);
            assert(a.safe_text + b.safe_text == "bye");
        }
    }

    //Test 8: misidentification test

    {
        SentinelScanner s("<END>");
        auto st1 = s.feed("<ENX>");
        auto st2 = s.flush();

        assert(!st1.sentinel_found);
        assert(st1.safe_text + st2.safe_text == "<ENX>");
    }

    //Test 9: scanner must not scan a larger or equal length string to the sentinel

    {
        std::string sen = "<|end_conversation|>";
        SentinelScanner s(sen);
        size_t fed = 0, out = 0;

        for (int i = 0; i < 4 * 1024 * 1024; ++i) {
            auto r = s.feed("<");
            out += r.safe_text.size();
            assert(++fed - out <= sen.size() - 1);
        }
    }

    //Test 10: Harness stops after max turns

    {
        std::ofstream("t.script") <<
            "role: assistant\none\n---\n"
            "role: assistant\ntwo\n";

        HarnessConfig cfg;
        cfg.max_turns = 2;
        Harness h(std::make_unique<ScriptedModelClient>("t.script"), { cfg });
        TestInput in( "a","b" );
        TestOutput out;

        assert(h.run(in, out).kind == StopReason::Kind::TurnLimit);
        std::remove("t.script");

    }

    //Test 11: Harness stops when sentinel is reached
    {
        std::ofstream("s.script") <<
            "chunk: 3\n"
            "role: assistant\nbye<|end_conversation|>\n";

        HarnessConfig cfg;
        cfg.max_turns = 5;
        Harness h(std::make_unique<ScriptedModelClient>("s.script"), { cfg});
        TestInput in( "hello" );
        TestOutput out;

        assert(h.run(in, out).kind == StopReason::Kind::Sentinel);
        std::remove("s.script");

    }

    //Test 12: replay messages

    {
        std::ofstream("t.txt") <<
            "role: user\nhello\n---\n"
            "role: assistant\nhi\n---\n"
            "role: user\nbye\n---\n"
            "role: assistant\ngoodbye\n";

        ReplayModelClient r("t.txt");
        Conversation c;

        assert(r.generate(c).content() == "hi");
        assert(r.generate(c).content() == "goodbye");

        std::remove("t.txt");
    }


    return 0;
}
