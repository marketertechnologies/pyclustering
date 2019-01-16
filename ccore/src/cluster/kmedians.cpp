/**
*
* @authors Andrei Novikov (pyclustering@yandex.ru)
* @date 2014-2019
* @copyright GNU Public License
*
* GNU_PUBLIC_LICENSE
*   pyclustering is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   pyclustering is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "cluster/kmedians.hpp"

#include <algorithm>
#include <cmath>

#include "utils/metric.hpp"


using namespace ccore::utils::metric;


namespace ccore {

namespace clst {


const double kmedians::THRESHOLD_CHANGE = 0.000001;


kmedians::kmedians(const dataset & initial_medians, const double tolerance, const distance_metric<point> & p_metric) :
    m_tolerance(tolerance),
    m_initial_medians(initial_medians),
    m_ptr_result(nullptr),
    m_ptr_data(nullptr),
    m_metric(p_metric)
{ }


void kmedians::process(const dataset & data, cluster_data & output_result) {
    m_ptr_data = &data;
    m_ptr_result = (kmedians_data *) &output_result;

    if (data[0].size() != m_initial_medians[0].size()) {
        throw std::invalid_argument("kmedians: dimension of the input data and dimension of the initial medians must be equal.");
    }

    m_ptr_result->medians() = m_initial_medians;

    double changes = 0.0;
    double prev_changes = 0.0;

    size_t counter_repeaters = 0;

    do {
        update_clusters(m_ptr_result->medians(), m_ptr_result->clusters());
        changes = update_medians(m_ptr_result->clusters(), m_ptr_result->medians());

        double change_difference = std::abs(changes - prev_changes);
        if (change_difference < THRESHOLD_CHANGE) {
            counter_repeaters++;
        }
        else {
            counter_repeaters = 0;
        }

        prev_changes = changes;
    }
    while ((changes > m_tolerance) && (counter_repeaters < 10));

    m_ptr_data = nullptr;
    m_ptr_result = nullptr;
}


void kmedians::update_clusters(const dataset & medians, cluster_sequence & clusters) {
    const dataset & data = *m_ptr_data;

    clusters.clear();
    clusters.resize(medians.size());

    for (size_t index_point = 0; index_point < data.size(); index_point++) {
        size_t index_cluster_optim = 0;
        double distance_optim = std::numeric_limits<double>::max();

        for (size_t index_cluster = 0; index_cluster < medians.size(); index_cluster++) {
            double distance = m_metric(data[index_point], medians[index_cluster]);
            if (distance < distance_optim) {
                index_cluster_optim = index_cluster;
                distance_optim = distance;
            }
        }

        clusters[index_cluster_optim].push_back(index_point);
    }

    erase_empty_clusters(clusters);
}


void kmedians::erase_empty_clusters(cluster_sequence & p_clusters) {
    for (size_t index_cluster = p_clusters.size() - 1; index_cluster != (size_t) -1; index_cluster--) {
        if (p_clusters[index_cluster].empty()) {
            p_clusters.erase(p_clusters.begin() + index_cluster);
        }
    }
}


double kmedians::update_medians(cluster_sequence & clusters, dataset & medians) {
    const dataset & data = *m_ptr_data;
    const size_t dimension = data[0].size();

    std::vector<point> prev_medians(medians);

    medians.clear();
    medians.resize(clusters.size(), point(dimension, 0.0));

    double maximum_change = 0.0;

    for (size_t index_cluster = 0; index_cluster < clusters.size(); index_cluster++) {
        calculate_median(clusters[index_cluster], medians[index_cluster]);

        double change = m_metric(prev_medians[index_cluster], medians[index_cluster]);
        if (change > maximum_change) {
            maximum_change = change;
        }
    }

    return maximum_change;
}


void kmedians::calculate_median(cluster & current_cluster, point & median) {
    const dataset & data = *m_ptr_data;
    const std::size_t dimension = data[0].size();

    for (size_t index_dimension = 0; index_dimension < dimension; index_dimension++) {
        std::sort(current_cluster.begin(), current_cluster.end(), 
            [this](std::size_t index_object1, std::size_t index_object2) 
        {
            return (*m_ptr_data)[index_object1] > (*m_ptr_data)[index_object2];
        });

        size_t relative_index_median = (size_t) floor((current_cluster.size() - 1) / 2.0);
        size_t index_median = current_cluster[relative_index_median];

        if (current_cluster.size() % 2 == 0) {
            size_t index_median_second = current_cluster[relative_index_median + 1];
            median[index_dimension] = (data[index_median][index_dimension] + data[index_median_second][index_dimension]) / 2.0;
        }
        else {
            median[index_dimension] = data[index_median][index_dimension];
        }
    }
}


}

}
