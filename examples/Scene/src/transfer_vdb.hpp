#pragma once

#if defined (WITH_OPENVDB)

#include <openvdb/openvdb.h>
#include <iostream>
#include <filesystem>
#include <glm/glm.hpp>
#include <stdio.h>

namespace MatchTF {
    template <class T>
    inline T lerp(T x, T y, float t) {
        return x + (y - x) * t;
    }

    template <class T>
    inline void save_vector(std::vector<T> &vec, const std::string &name) {
        if constexpr (std::is_floating_point_v<T>) {
            auto max = *std::max_element(vec.begin(), vec.end());
            for (auto &v : vec) {
                v /= max;
            }
        }
        FILE *f = fopen((name + ".match_volume_data").c_str(), "wb");
        fwrite(vec.data(), sizeof(T), vec.size(), f);
        fclose(f);
    }

    inline void initialize() {
        openvdb::initialize();
    }

    inline void transfer(const std::string &filename, size_t volume_raw_data_resolution) {
        openvdb::io::File file(filename);
        file.open();

        for (openvdb::io::File::NameIterator name_iter = file.beginName(); name_iter != file.endName(); ++name_iter) {
            openvdb::GridBase::Ptr base_grid;
            base_grid = file.readGrid(name_iter.gridName());
            std::cout << std::endl << name_iter.gridName() << ":===:" << base_grid->valueType() << std::endl;

            auto fn = std::filesystem::path(filename).filename().string();
            auto p = fn.find_last_of(".");
            auto name = fn.substr(0, p) + "_" + name_iter.gridName();

            glm::ivec3 bmin_glm;
            glm::vec3 extent;

            if (base_grid->valueType() == "float" || base_grid->valueType() == "vec3s") {
                for (auto iter = base_grid->beginMeta(); iter != base_grid->endMeta(); ++iter) {
                    const std::string& name = iter->first;
                    openvdb::Metadata::Ptr value = iter->second;

                    std::cout << name << " = " << value->str() << " -- " << value->typeName() << std::endl;
                }

                auto bmax = base_grid->getMetadata<openvdb::Vec3IMetadata>("file_bbox_max");
                auto bmin = base_grid->getMetadata<openvdb::Vec3IMetadata>("file_bbox_min");
                auto bmax_ = bmax.get()->value();
                auto bmin_ = bmin.get()->value();

                glm::ivec3 bmax_glm = { bmax_[0], bmax_[1], bmax_[2] };
                bmin_glm = { bmin_[0], bmin_[1], bmin_[2] };
                extent = bmax_glm - bmin_glm;
                float max = glm::max(glm::max(extent.x, extent.y), extent.z);
                auto extent_bounded = extent / max;
                std::cout << max << "  (((())))  " << volume_raw_data_resolution << std::endl;
                std::cout << extent_bounded.x << " " << extent_bounded.y << " " << extent_bounded.z << std::endl;
            } else {
                continue;
            }
            if (base_grid->valueType() == "float") {
                openvdb::FloatGrid::Ptr grid = openvdb::gridPtrCast<openvdb::FloatGrid>(base_grid);
                auto accessor = grid->getConstAccessor();

                std::vector<float> raw_data(volume_raw_data_resolution * volume_raw_data_resolution * volume_raw_data_resolution);

                for (int x_i = 0; x_i < volume_raw_data_resolution; x_i ++) {
                    for (int y_i = 0; y_i < volume_raw_data_resolution; y_i ++) {
                        for (int z_i = 0; z_i < volume_raw_data_resolution; z_i ++) {
                            auto pos = extent * glm::vec3 { x_i, y_i, z_i } / float(volume_raw_data_resolution) + glm::vec3(bmin_glm);
                            auto pos_min = glm::floor(pos);
                            auto pos_f = glm::fract(pos);
                            float v000 = accessor.getValue({
                                static_cast<int>(pos_min.x),
                                static_cast<int>(pos_min.y),
                                static_cast<int>(pos_min.z),
                            });
                            float v001 = accessor.getValue({
                                static_cast<int>(pos_min.x),
                                static_cast<int>(pos_min.y),
                                static_cast<int>(pos_min.z + 1),
                            });
                            float v010 = accessor.getValue({
                                static_cast<int>(pos_min.x),
                                static_cast<int>(pos_min.y + 1),
                                static_cast<int>(pos_min.z),
                            });
                            float v011 = accessor.getValue({
                                static_cast<int>(pos_min.x),
                                static_cast<int>(pos_min.y + 1),
                                static_cast<int>(pos_min.z + 1),
                            });
                            float v100 = accessor.getValue({
                                static_cast<int>(pos_min.x + 1),
                                static_cast<int>(pos_min.y),
                                static_cast<int>(pos_min.z),
                            });
                            float v101 = accessor.getValue({
                                static_cast<int>(pos_min.x + 1),
                                static_cast<int>(pos_min.y),
                                static_cast<int>(pos_min.z + 1),
                            });
                            float v110 = accessor.getValue({
                                static_cast<int>(pos_min.x + 1),
                                static_cast<int>(pos_min.y + 1),
                                static_cast<int>(pos_min.z),
                            });
                            float v111 = accessor.getValue({
                                static_cast<int>(pos_min.x + 1),
                                static_cast<int>(pos_min.y + 1),
                                static_cast<int>(pos_min.z + 1),
                            });

                            float sample_value = lerp(
                                lerp(
                                    lerp(v000, v001, pos_f.z),
                                    lerp(v010, v011, pos_f.z),
                                    pos_f.y
                                ),
                                lerp(
                                    lerp(v100, v101, pos_f.z),
                                    lerp(v110, v111, pos_f.z),
                                    pos_f.y
                                ), pos_f.x
                            );

                            raw_data.at(x_i * volume_raw_data_resolution * volume_raw_data_resolution + y_i * volume_raw_data_resolution + z_i) = sample_value;
                        }
                    }
                }

                save_vector(raw_data, name);
            } else if (base_grid->valueType() == "vec3s") {
                openvdb::Vec3SGrid::Ptr grid = openvdb::gridPtrCast<openvdb::Vec3SGrid>(base_grid);
                auto accessor = grid->getConstAccessor();
                std::vector<glm::vec3> raw_data(volume_raw_data_resolution * volume_raw_data_resolution * volume_raw_data_resolution);

                for (int x_i = 0; x_i < volume_raw_data_resolution; x_i ++) {
                    for (int y_i = 0; y_i < volume_raw_data_resolution; y_i ++) {
                        for (int z_i = 0; z_i < volume_raw_data_resolution; z_i ++) {
                            auto pos = extent * glm::vec3 { x_i, y_i, z_i } / float(volume_raw_data_resolution) + glm::vec3(bmin_glm);
                            auto pos_min = glm::floor(pos);
                            auto pos_f = glm::fract(pos);
                            openvdb::math::Vec3<float> v000 = accessor.getValue({
                                static_cast<int>(pos_min.x),
                                static_cast<int>(pos_min.y),
                                static_cast<int>(pos_min.z),
                            });
                            openvdb::math::Vec3<float> v001 = accessor.getValue({
                                static_cast<int>(pos_min.x),
                                static_cast<int>(pos_min.y),
                                static_cast<int>(pos_min.z + 1),
                            });
                            openvdb::math::Vec3<float> v010 = accessor.getValue({
                                static_cast<int>(pos_min.x),
                                static_cast<int>(pos_min.y + 1),
                                static_cast<int>(pos_min.z),
                            });
                            openvdb::math::Vec3<float> v011 = accessor.getValue({
                                static_cast<int>(pos_min.x),
                                static_cast<int>(pos_min.y + 1),
                                static_cast<int>(pos_min.z + 1),
                            });
                            openvdb::math::Vec3<float> v100 = accessor.getValue({
                                static_cast<int>(pos_min.x + 1),
                                static_cast<int>(pos_min.y),
                                static_cast<int>(pos_min.z),
                            });
                            openvdb::math::Vec3<float> v101 = accessor.getValue({
                                static_cast<int>(pos_min.x + 1),
                                static_cast<int>(pos_min.y),
                                static_cast<int>(pos_min.z + 1),
                            });
                            openvdb::math::Vec3<float> v110 = accessor.getValue({
                                static_cast<int>(pos_min.x + 1),
                                static_cast<int>(pos_min.y + 1),
                                static_cast<int>(pos_min.z),
                            });
                            openvdb::math::Vec3<float> v111 = accessor.getValue({
                                static_cast<int>(pos_min.x + 1),
                                static_cast<int>(pos_min.y + 1),
                                static_cast<int>(pos_min.z + 1),
                            });

                            openvdb::math::Vec3<float> sample_value = lerp(
                                lerp(
                                    lerp(v000, v001, pos_f.z),
                                    lerp(v010, v011, pos_f.z),
                                    pos_f.y
                                ),
                                lerp(
                                    lerp(v100, v101, pos_f.z),
                                    lerp(v110, v111, pos_f.z),
                                    pos_f.y
                                ), pos_f.x
                            );

                            raw_data.at(x_i * volume_raw_data_resolution * volume_raw_data_resolution + y_i * volume_raw_data_resolution + z_i) = glm::vec3 { sample_value.x(), sample_value.y(), sample_value.z() };
                        }
                    }
                }

                save_vector(raw_data, name);
            }
        }

        file.close();
    }
}

#else

#include <string>
#include <iostream>

namespace MatchTF {
    inline void initialize() {
        std::cout << "Could not find OpenVDB" << std::endl;
        std::cout << "Initialize faild." << std::endl;
    }

    inline void transfer(const std::string &filename, size_t) {
        std::cout << "Could not find OpenVDB" << std::endl;
        std::cout << "Transfer " << filename << " faild." << std::endl;
    }
}

#endif
