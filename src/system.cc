module;

#include <algorithm>
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <cassert>
#include <span>

#include <print>
#include <arm_neon.h>
#include <thread>


export module ecstasy:system;

import :component;
import :entity;

import util;

export namespace ecstasy {
  using namespace component;
  namespace systems {
    using namespace ecstasy::component;

    void moveTransform(CTransform2D& t, CVelocity& v) {
      t.position.x += v.velocity.x;
      t.position.y += v.velocity.y;
    }

    void moveTransformPar(const std::vector<Entity>& es, int t, int nt) {
      size_t i = (es.size() / nt) * t;
      size_t end = i + (es.size() / nt);

      for(; i < end; i++) {
        component_manager.get<CTransform2D>(es[i]).position.x += component_manager.get<CVelocity>(es[i]).velocity.x;
        component_manager.get<CTransform2D>(es[i]).position.y += component_manager.get<CVelocity>(es[i]).velocity.y;
      }
    }

    void move_transform(const std::span<Entity> es) {
      for(auto i : es) {
        component_manager.get<CTransform2D>(i).position = component_manager.get<CTransform2D>(i).position + component_manager.get<CVelocity>(i).velocity;
      }
    }

    void remove_out_of_bounds(const std::span<Entity> es) {
      for(auto i: es) {
        auto pos = component_manager.get<CTransform2D>(i).position;
        if(pos.x < 0 ||
           pos.y < 0 ||
           pos.x > 1920 ||
           pos.y > 1080) {
          deads.push(i);
        }
      }
    }

    void orient_to_attractor(const std::span<Entity> es) {
      for(auto i: es) {
        const auto att = component_manager.get<CAttraction>(i).attractor;
        auto entity_pos = component_manager.get<CTransform2D>(i).position;
        auto attractor_pos = component_manager.get<CTransform2D>(att).position;

        if(attractor_pos.x == component_manager.get<CAttraction>(i).cache.x &&
            attractor_pos.y == component_manager.get<CAttraction>(i).cache.y) {
          continue;
        }
        component_manager.get<CAttraction>(i).cache = attractor_pos;
        auto dvec = attractor_pos - entity_pos;
        auto val = (dvec.x * dvec.x) + (dvec.y * dvec.y);
        float dist = sqrtf(val);
        float cur_power = component_manager.get<CAttraction>(i).gravity / dist;
        component_manager.get<CVelocity>(i).velocity.x = (dvec.x/dist) * cur_power;
        component_manager.get<CVelocity>(i).velocity.y = (dvec.y/dist) * cur_power;
      }
    }

    void check_collisions_with_single_entity(const std::span<Entity> es, const Entity e) {
      const auto& te = component_manager.get<CTransform2D>(e);
      Circle ce(te.position, te.scale.x * 0.5f);
      for(auto i: es) {
        const auto& ti = component_manager.get<CTransform2D>(i);
        Circle ci(ti.position, ti.scale.x * 0.5f);
        if(ci.collides_with(ce)) {
          component_manager.get<CCollider>(i).callback(i, e);
        }
      }
    }

    void remove_orphans(Entity e) {
      for(auto &c: component_manager.get<CChildren>(e).children) {
        deads.push(c);
      }
    }

    void remove_from_family(Entity e) {
      auto &parent = component_manager.get<CParent>(e).parent;
      auto &siblings = component_manager.get<CChildren>(parent).children;
      siblings.erase(std::remove(siblings.begin(), siblings.end(), e), siblings.end());
    }

    void remove_deads(EntityManager& em) {
      auto ch_ents = em.get_associated_entities<CChildren>();
      auto par_ents = em.get_associated_entities<CParent>();
      while(!deads.empty()) {
        auto e = deads.front();
        if(std::find(ch_ents.begin(), ch_ents.end(), e) != ch_ents.end()) {
          remove_orphans(e);
        }
        if(std::find(par_ents.begin(), par_ents.end(), e) != par_ents.end()) {
          remove_from_family(e);
        }
        deads.pop();
        em.delete_entity(e);
        reuse.push(e);
      }
    }

    template<typename T>
    concept AssetManager = requires(T manager) {
      manager.sprites["orb"];
      manager.scripts["asd"];
    };

    template<AssetManager T>
    void progress_script(std::span<Entity> es, EntityManager& em, T assets) {

      using sc = ecstasy::component::CScript;
      for(auto e: es) {
        auto &s_component = component_manager.get<CScript>(e);

        if(s_component.pc >= s_component.program.size()) {
          deads.push(e);
          continue;
        }

        if(s_component.waitctr > 0) {
          s_component.waitctr--;
        } else {
          if(s_component.state == sc::VMState::MOVING) {
            component_manager.get<CVelocity>(e).velocity = {0, 0};
          }
          auto player = component_manager.get<CTransform2D>(1).position;

          Vec2 enm_pos;
          if(s_component.state == sc::VMState::SUBTHREAD) {
            auto par = component_manager.get<component::CParent>(e).parent;
            enm_pos = component_manager.get<CTransform2D>(par).position;
          } else {
            enm_pos = component_manager.get<CTransform2D>(e).position;
          }

          auto op = s_component.consume_opcode();

          // std::println("process #{} pc: {:x}. OP: {:x}, stack size: {}", e, s_component.pc, std::to_underlying(op), s_component.memory.size());
          switch(op) {
            case OpCode::CALL:
              {
              s_component.call_stack.push_back(
                  {
                  s_component.pc + static_cast<u32>(sizeof(i32)),
                  });
              s_component.pc = s_component.consume_operand() ;
              }
              break;

            case OpCode::RETURN:
              {
                if(s_component.call_stack.size()) {
                  s_component.pc = s_component.call_stack.back();
                  s_component.call_stack.pop_back();
                } else {
                  deads.push(e);
                }
              }
              break;

            case OpCode::DELETE:
              deads.push(e);
              break;

            case OpCode::WAIT:
              s_component.waitctr = s_component.consume_operand();
              break;

            case OpCode::PUSHI:
            {
              auto slot = component::CScript::StackSlot(s_component.consume_operand());
              s_component.memory.push_back(slot);
              break;
            }

            case OpCode::SETI:
            {
              auto regno = s_component.program[s_component.pc] & 0x000000FF;
              s_component.regs[regno] = s_component.memory.back().get<i32>();
              s_component.memory.pop_back();
              s_component.pc += sizeof(i32);
            }
            break;

            case OpCode::POP:
              s_component.memory.pop_back();
              break;

            case OpCode::ADDI:
            {
              i32 sum = 0;
              if(s_component.memory.size() >= 2) {
                sum += s_component.memory.back().get<i32>();
                s_component.memory.pop_back();
                sum += s_component.memory.back().get<i32>();
                s_component.memory.pop_back();
              } else if(s_component.memory.size() == 1) {
                // s_component.pc++;
                break;
              }              
              s_component.memory.push_back(component::CScript::StackSlot(sum));
              // s_component.pc++;
              break;
            }

            case OpCode::SUBI:
            {
              if(s_component.memory.size() >= 2) {
                i32 res = s_component.memory.back().get<i32>();
                s_component.memory.pop_back();
                res -= s_component.memory.back().get<i32>();
                s_component.memory.pop_back();
                s_component.memory.push_back(component::CScript::StackSlot(res));
              }
              break;
            }

            case OpCode::DIVI:
            {
              if(s_component.memory.size() >= 2) {
                auto x = s_component.memory.back().get<i32>();
                s_component.memory.pop_back();
                auto y = s_component.memory.back().get<i32>();
                s_component.memory.pop_back();
                auto res = x / y;
                s_component.memory.push_back(component::CScript::StackSlot(res));
              }
              break;
            }

            case OpCode::MULI:
            {
              if(s_component.memory.size() >= 2) {
                auto x = s_component.memory.back().get<i32>();
                s_component.memory.pop_back();
                auto y = s_component.memory.back().get<i32>();
                s_component.memory.pop_back();
                auto res = x * y;
                s_component.memory.push_back(component::CScript::StackSlot(res));
              }
              break;
            }

            case OpCode::JMP:
              s_component.pc = s_component.consume_operand();
              break;

            case OpCode::JMPEQ:
              {
              auto addr = s_component.consume_operand();
              if(s_component.memory.size()) {
                auto x = s_component.memory.back();
                if(x.get<i32>() == 0) {
                  s_component.pc = addr;
                }
              } else {
                auto addr = s_component.consume_operand();
                s_component.pc = addr;
              }
              break;
              }
            case OpCode::JMPNEQ:
              {
              auto addr = s_component.consume_operand();
              if(s_component.memory.size()) {
                auto x = s_component.memory.back();

                if(x.get<i32>() != 0) {
                  s_component.pc = addr;
                }
              }
              break;
              }
            case OpCode::CALLASYNC:
              {
              auto as = em.create_entity();
              auto tmp = CScript(s_component.program);
              auto addr = s_component.consume_operand();
              tmp.init_from_parent(addr);
              tmp.state = sc::VMState::SUBTHREAD;

              em.add_components<CScript, CParent, CChildren>(as,
                std::move(tmp),
                {e},
                {}
              );

              auto &c = component_manager.get<CChildren>(e);
              c.children.push_back(as);
              break;
              }
            case OpCode::CALLASYNCID:
              break;

            case OpCode::ENMCREATE: 
              {
                auto enm = em.create_entity();
                auto tmp = CScript(s_component.program);
                auto addr = s_component.consume_operand();
                tmp.init_from_parent(addr);

                f32 x = s_component.consume_operand();
                f32 y = s_component.consume_operand();
                [[maybe_unused]] auto health = s_component.consume_operand();
                [[maybe_unused]] auto score = s_component.consume_operand();
                [[maybe_unused]] auto items = s_component.consume_operand();

                auto sprite = &assets.sprites["enm1.png"].data;
                em.add_components<CTransform2D, CVelocity, CSprite, CScript, CBulletManager, CHealth>(enm,
                    { .position { .x = x, .y = y }, .scale { 1, 1 }, .rotation = 0.f },
                    { .velocity { .x = 0, .y = 0 } },
                    { .sprite = sprite },
                    std::move(tmp),
                    {},
                    {health}
                    );
                break;
              }

            case OpCode::ETNEW: 
            {
              auto slot = s_component.consume_operand();
              assert(slot < 16);
              new(&component_manager.get<CBulletManager>(e).patterns[slot]) CBulletManager();
              break;
            }
            case OpCode::ETON:
            {
              auto slot = s_component.consume_operand();
              Vec2 vel{};
              auto& bp = component_manager.get<CBulletManager>(e).patterns[slot];

              auto sprite = &assets.sprites["orb1.png"].data;
              auto dvec = player - enm_pos;
              auto aim_angle = std::atan2(dvec.y, dvec.x);
              switch(bp.mode) {
                case BulletMode::BM_CIRC_AIM:
                  {
                  auto ang_step =  360.f / bp.columns;
                  for(auto i = 0; i < bp.rows; ++i) {
                    float lspeed = (bp.speed1 + bp.speed2) + ((bp.speed1 - bp.speed2) / (float(bp.rows) / (i)));
                    for(auto j = 0; j < bp.columns; ++j) {
                      auto angle_diff = ang_step * j;
                      auto new_angle = aim_angle + (angle_diff * (PI / 180));
                      vel.x = std::cos(new_angle + (i * bp.angle2 )) * lspeed * 0.005; 
                      vel.y = std::sin(new_angle + (i * bp.angle2 )) * lspeed * 0.005;
                      auto bullet = em.create_entity();
                      em.add_components<CTransform2D, CVelocity, CSprite>(bullet,
                        {
                          enm_pos,
                          {
                            static_cast<float>(sprite->width),
                            static_cast<float>(sprite->height)
                          },
                          component_manager.get<CBulletManager>(e).patterns[slot].angle1,
                        }, {
                          vel
                        }, {
                          sprite
                        }
                      );
                    }
                  }
                  break;
                }
                case BulletMode::BM_NORM_AIM:
                  {
                  auto even_diff = bp.columns & 1 ? 0 : 0.5;
                  for(auto i = 0; i < bp.rows; ++i) {
                    float lspeed = (bp.speed1 + bp.speed2) + ((bp.speed1 - bp.speed2) / (float(bp.rows) / (i)));
                    for(auto j = 0; j < bp.columns; ++j) {
                      auto diff = (j + 1) - round(0.5 * bp.columns);
                      auto new_angle = aim_angle + (diff * bp.angle2) - (even_diff * bp.angle2) + bp.angle1;
                      vel.x = std::cos(new_angle) * lspeed * 0.005;
                      vel.y = std::sin(new_angle) * lspeed * 0.005;
                      auto bullet = em.create_entity();
                      em.add_components<CTransform2D, CVelocity, CSprite>(bullet,
                        {
                          enm_pos,
                          {
                            static_cast<float>(sprite->width),
                            static_cast<float>(sprite->height)
                          },
                          component_manager.get<CBulletManager>(e).patterns[slot].angle1,
                        }, {
                          vel
                        }, {
                          sprite
                        }
                      );
                    }
                  }
                  break;
                }

                default:
                  break;
              }
            }

            case OpCode::ETSPRITE:
              break;

            case OpCode::ETOFFSET:
              break;

            case OpCode::ETANGLE: {
              auto slot = s_component.consume_operand();
              auto angle1 = s_component.consume_operand();
              auto angle2 = s_component.consume_operand();

              auto& bp = component_manager.get<CBulletManager>(e).patterns[slot];
              bp.angle1 = angle1 * (PI / 180);
              bp.angle2 = angle2 * (PI / 180);
              break;
            }
            case OpCode::ETSPEED: {
              auto slot = s_component.consume_operand();
              auto speed1 = s_component.consume_operand();
              auto speed2 = s_component.consume_operand();
              auto& bp = component_manager.get<CBulletManager>(e).patterns[slot];
              bp.speed1 = speed1;
              bp.speed2 = speed2;
              break;
            }
            case OpCode::ETCOUNT: {
              auto slot = s_component.consume_operand();
              auto rows = s_component.consume_operand();
              auto columns = s_component.consume_operand();

              auto& bp = component_manager.get<CBulletManager>(e).patterns[slot];
              bp.rows = rows;
              bp.columns = columns;
              break;
            }
            case OpCode::ETAIM: 
            {
              auto slot = s_component.consume_operand();
              auto mode = s_component.consume_operand();

              auto& bp = component_manager.get<CBulletManager>(e);
              bp.patterns[slot].mode = static_cast<BulletMode>(mode);
              break;
            }
            case OpCode::MOVEPOS: 
            {
              auto x = s_component.consume_operand();
              auto y = s_component.consume_operand();
              enm_pos = {
                static_cast<f32>(x),
                static_cast<f32>(y)
              };
              break;
            }

            case OpCode::MOVEPOSTIME: 
            {
              auto time = s_component.consume_operand();
              [[maybe_unused]] auto mode = s_component.consume_operand();
              auto x = s_component.consume_operand();
              auto y = s_component.consume_operand();

              auto delta =
                  Vec2{static_cast<f32>(x), static_cast<f32>(y)} -
                  component_manager.get<CTransform2D>(e).position;

              component_manager.get<CVelocity>(e).velocity = {
                .x = (delta.x / time),
                .y = (delta.y / time)
              };
              s_component.waitctr = time;
              s_component.state = sc::VMState::MOVING;
              break;
            }

            case OpCode::PRINT:
            {
              auto &s = s_component.memory;
              for(auto i: s) {
                std::print("{}, ", i.get<i32>());
              }
              std::println("");
              break;
            }

            case OpCode::PRINTR:
            {
              auto regno = s_component.program[s_component.pc] & 0x000000FF;
              auto val = s_component.regs[regno].get<i32>();
              std::println("$r{}: {}", regno, val);
              s_component.pc += sizeof(i32);
              break;
            }
            
            default:
              std::println("Unimplmented Opcode {:x} found @{:x}", std::to_underlying(op), s_component.pc);
              deads.push(e);
          }
        }
      }
    }
  }
}
