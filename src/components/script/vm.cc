module;

#include <vector>
#include <print>
#include <bitset>
#include <variant>

export module ecstasy.components.script.vm;

import util;

import ecstasy.components.script.opcodes;
import ecstasy.components.bullets;

export namespace ecstasy {
  namespace component {

    template<typename T>
    concept is_vm_type = requires {
      requires (std::is_same_v<T, i32> || std::is_same_v<T, f32>);
    };

    struct CScript {
      CScript() = default;
      CScript(CScript const &other) : program(other.program) {
        init_from_script();
      };

      CScript(const std::vector<u8> code) : program(code) {
        init_from_script();
      }

      enum DataType {
        Integer,
        Float,
      };

      struct StackSlot {

        StackSlot(i32 data) : m_data(data), m_type(Integer) {}
        StackSlot(f32 data) : m_data(data), m_type(Float) {}

        StackSlot() = default;

        template<typename T>
        requires(is_vm_type<T>)
        [[nodiscard]]
        T get() {
          return std::get<T>(m_data);
        }

        std::variant<i32, f32> m_data;
        DataType m_type;
      };

      void init_from_script() {
        constexpr std::array magic = { 0x7f, 0x44, 0x4d, 0x4c };
        for(int i = 0; i < 4; ++i) {
          if(magic[i] != program[i]) {
            std::println("Unrecognized file format.");
            break;
          }
        }
        // Set pc to the entry point found in header file
        pc += (program[4]) & 0x000000FF;
        pc += (program[5] << 8) & 0x0000FF00;
        pc += (program[6] << 16) & 0x00FF0000;
        pc += (program[7] << 24) & 0xFF000000;
      }

      void init_from_parent(size_t s_addr) {
        pc = s_addr;
      }

      template<typename T = int>
      [[nodiscard]]
      T get_operand() noexcept {
        auto flags = (program[pc+3]);
        switch(flags) {
          case 0: 
            {
            return get_int_operand();
            }
          case 1:
            {
            auto var = program[pc];
            return regs[var].get<int>();
            }
          case 2:
            {
            return get_int_operand();
            }
          default:
            std::println("UNABLE TO DECODE");
            return 0;
        }
      }

      [[nodiscard]]
      float get_float_operand() noexcept {
        std::vector<u8> num_str(program.begin() + pc ,program.begin() + pc + 4);
        float res = 0;
        memcpy(&res, num_str.data(), 4);
        return res;
      }

      [[nodiscard]]
      int get_int_operand() noexcept {
        int num = 0;
        num += program[pc] & 0x000000FF;
        num += (program[pc+1] << 8) & 0x0000FF00;
        num += (program[pc+2] << 16) & 0x00FF0000;
        return num;
      }

      template<typename T = int>
      [[nodiscard]]
      T consume_operand() noexcept {
        auto flags = (program[pc+3]);
        switch(flags) {
          case 0: 
            {
            return consume_int_operand();
            }
          case 1:
            {
            auto var = program[pc];
            pc += sizeof(i32);
            return regs[var].get<int>();
            }
          case 2:
            {
            return consume_int_operand();
            }
          default:
            std::println("UNABLE TO DECODE");
            return 0;
        }
      }

      [[nodiscard]]
      int consume_int_operand() noexcept {
        int num = 0;
        num += program[pc++] & 0x000000FF;
        num += (program[pc++] << 8) & 0x0000FF00;
        num += (program[pc++] << 16) & 0x00FF0000;
        pc++;
        return num;
      }

      OpCode consume_opcode() {
        int op = program[pc];
        op = op << 8;
        pc++;
        op |= program[pc];
        auto opcode = static_cast<OpCode>(op);
        pc++;
        return opcode;
      }

      enum class VMState : uint8_t {
        MOVING,
        SUBTHREAD,
        SIZE
      };

      u16 pc = 0;
      u16 sp = 0;
      u16 waitctr = 0;
      VMState state;
      std::vector<u8> program;
      std::vector<StackSlot> memory;
      std::vector<u32> call_stack;
      std::array<StackSlot, 16> regs;
    };
  }
}
