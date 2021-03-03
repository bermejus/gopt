#pragma once

#include <random>
#include <vector>
#include <limits>
#include "vector.hpp"
#include <iostream>
#include <sstream>

#include <matplotlibcpp.h>
namespace plt = matplotlibcpp;

namespace gopt
{
	template <typename T, unsigned int S>
	struct Particle
	{
		Vector_t<T, S> position;
		Vector_t<T, S> velocity;
		Vector_t<T, S> best_position;
		T best_score;
	};

	template <typename T, unsigned int S, typename F>
	Vector_t<T, S> particleswarm(F& f, const Vector_t<T, S>& lb, const Vector_t<T, S>& ub, const unsigned int n_particles = 100*S, const unsigned int max_iter = 1000)
	{
		std::default_random_engine gen;
		std::uniform_real_distribution<> dist(0.0, 1.0);

		const T w = 0.9;
		const T c1 = 1;
		const T c2 = 1;
		const unsigned int max_iter_wo_change = 0.3 * max_iter;

		T g_best_score = std::numeric_limits<T>::max();
		Vector_t<T, S> g_best_position;

		std::vector<T> score_array;
		std::vector<T> indices_array;
		score_array.reserve(max_iter);
		indices_array.reserve(max_iter);

		// Initialize random particles
		std::vector<Particle<T, S>> particles(n_particles);

		for (int i = 0; i < n_particles; i++)
		{
			Particle<T, S>& p = particles[i];

			for (int j = 0; j < S; j++)
				p.position[j] = lb[j] + (ub[j] - lb[j]) * dist(gen);

			p.velocity = 0;
			p.best_position = p.position;
			p.best_score = g_best_score;
		}

		unsigned int iter_wo_change = 0;
		for (int k = 0; k < max_iter; k++)
		{
			const T old_score = g_best_score;

			#pragma omp parallel for
			for (int i = 0; i < n_particles; i++)
			{
				auto& p = particles[i];

				const T score = f(p.position);
				if (score < p.best_score)
				{
					p.best_score = score;
					p.best_position = p.position;

					#pragma omp critical
					if (score < g_best_score)
					{
						g_best_score = score;
						g_best_position = p.position;
					}
				}
			}

			if (g_best_score == old_score)
			{
				iter_wo_change++;
				if (iter_wo_change > max_iter_wo_change)
					break;
			}
			else
				iter_wo_change = 0;

			std::cout << " Iter: " << k << ", best score: " << g_best_score << std::endl;

			score_array.push_back(g_best_score);
			indices_array.push_back(k + 1);

			// Update particles position
			#pragma omp parallel for
			for (int i = 0; i < n_particles; i++)
			{
				auto& p = particles[i];
				p.velocity = w * p.velocity + (c1 * dist(gen)) * (p.best_position - p.position) + (c2 * dist(gen)) * (g_best_position - p.position);
				p.position += p.velocity;

				for (int j = 0; j < S; j++)
				{
					const T space = ub[j] - lb[j];

					if (p.velocity[j] > space)
						p.velocity[j] = 0;

					else if (p.position[j] > ub[j] || p.position[j] < lb[j])
						p.velocity[j] = -p.velocity[j];

					p.position[j] = std::clamp(p.position[j], lb[j], ub[j]);
				}
			}
		}

		plt::figure(1);
		plt::plot(indices_array, score_array, "*");
		plt::title(std::string("Best score: ") + std::to_string(g_best_score));
		plt::xlim(indices_array.front(), indices_array.back());
		plt::grid(true);

		return g_best_position;
	}
}