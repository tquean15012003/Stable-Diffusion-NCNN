#include "diffusion_slover.h"
#include "latent_utils.h"

int DiffusionSlover::load(AAssetManager* mgr, std::string bin)
{
	ncnn::set_cpu_powersave(2);
	ncnn::set_omp_num_threads(ncnn::get_big_cpu_count());

	net.opt.lightmode = true;
	net.opt.use_vulkan_compute = false;
	net.opt.use_winograd_convolution = false;
	net.opt.use_sgemm_convolution = false;
	net.opt.use_fp16_packed = true;
	net.opt.use_fp16_storage = true;
	net.opt.use_fp16_arithmetic = true;
	net.opt.use_packing_layout = true;
	net.opt.num_threads = ncnn::get_big_cpu_count();

	net.load_param(mgr,"UNetModel-256-MHA-fp16-opt.param");
	net.load_model(mgr,"UNetModel-MHA-fp16.bin");

	ifstream in(bin, ios::in | ios::binary);
	in.read((char*)&log_sigmas, sizeof log_sigmas);
	in.close();

	return 0;
}

ncnn::Mat DiffusionSlover::randn_4_32_32(int seed)
{
	cv::Mat cv_x(cv::Size(32, 32), CV_32FC4);
	cv::RNG rng(seed);
	rng.fill(cv_x, cv::RNG::NORMAL, 0, 1);
	ncnn::Mat x_mat(32, 32, 4, (void*)cv_x.data);
	return x_mat.clone();
}

ncnn::Mat DiffusionSlover::CFGDenoiser_CompVisDenoiser(ncnn::Mat& input, float sigma, ncnn::Mat cond, ncnn::Mat uncond)
{
	// get_scalings
	float c_out = -1.0 * sigma;
	float c_in = 1.0 / sqrt(sigma * sigma + 1);

	// sigma_to_t
	float log_sigma = log(sigma);
	vector<float> dists(1000);
	for (int i = 0; i < 1000; i++) {
		if (log_sigma - log_sigmas[i] >= 0)
			dists[i] = 1;
		else
			dists[i] = 0;
		if (i == 0) continue;
		dists[i] += dists[i - 1];
	}
	int low_idx = min(int(max_element(dists.begin(), dists.end()) - dists.begin()), 1000 - 2);
	int high_idx = low_idx + 1;
	float low = log_sigmas[low_idx];
	float high = log_sigmas[high_idx];
	float w = (low - log_sigma) / (low - high);
	w = max(0.f, min(1.f, w));
	float t = (1 - w) * low_idx + w * high_idx;

	ncnn::Mat t_mat(1);
	t_mat[0] = t;

	ncnn::Mat c_in_mat(1);
	c_in_mat[0] = c_in;

	ncnn::Mat c_out_mat(1);
	c_out_mat[0] = c_out;

	ncnn::Mat v44;
	ncnn::Mat v83;
	ncnn::Mat v116;
	ncnn::Mat v163;
	ncnn::Mat v251;
	ncnn::Mat v337;
	ncnn::Mat v425;
	ncnn::Mat v511;
	ncnn::Mat v599;
	ncnn::Mat v627;
	ncnn::Mat v711;
	ncnn::Mat v725;
	ncnn::Mat v740;
	ncnn::Mat v755;
	ncnn::Mat v772;
	ncnn::Mat v858;
	ncnn::Mat v944;
	ncnn::Mat v1032;
	ncnn::Mat v1118;
	ncnn::Mat v1204;
	ncnn::Mat v1292;
	ncnn::Mat v1378;
	ncnn::Mat v1464;

	ncnn::Mat denoised_cond;
	{
		ncnn::Extractor ex = net.create_extractor();
		ex.set_light_mode(true);
		ex.input("in0", input);
		ex.input("in1", t_mat);
		ex.input("in2", cond);
		ex.input("c_in", c_in_mat);
		ex.input("c_out", c_out_mat);
		ex.extract("44", v44, 1);
		ex.extract("83", v83, 1);
		ex.extract("116", v116, 1);
		ex.extract("163", v163, 1);
		ex.extract("251", v251, 1);
		ex.extract("337", v337, 1);
		ex.extract("425", v425, 1);
		ex.extract("511", v511, 1);
		ex.extract("599", v599, 1);
		ex.extract("627", v627, 1);
		ex.extract("711", v711, 1);
		ex.extract("725", v725, 1);
		ex.extract("740", v740, 1);
		ex.extract("755", v755, 1);
		ex.extract("772", v772, 1);
		ex.extract("858", v858, 1);
		ex.extract("944", v944, 1);
		ex.extract("1032", v1032, 1);
		ex.extract("1118", v1118, 1);
		ex.extract("1204", v1204, 1);
		ex.extract("1292", v1292, 1);
		ex.extract("1378", v1378, 1);
		ex.extract("1464", v1464, 1);
		ex.extract("outout", denoised_cond);
	}

	ncnn::Mat denoised_uncond;
	{
		ncnn::Extractor ex = net.create_extractor();
		ex.set_light_mode(true);
		ex.input("in0", input);
		ex.input("in1", t_mat);
		ex.input("in2", uncond);
		ex.input("c_in", c_in_mat);
		ex.input("c_out", c_out_mat);
		ex.input("44", v44);
		ex.input("83", v83);
		ex.input("116", v116);
		ex.input("163", v163);
		ex.input("251", v251);
		ex.input("337", v337);
		ex.input("425", v425);
		ex.input("511", v511);
		ex.input("599", v599);
		ex.input("627", v627);
		ex.input("711", v711);
		ex.input("725", v725);
		ex.input("740", v740);
		ex.input("755", v755);
		ex.input("772", v772);
		ex.input("858", v858);
		ex.input("944", v944);
		ex.input("1032", v1032);
		ex.input("1118", v1118);
		ex.input("1204", v1204);
		ex.input("1292", v1292);
		ex.input("1378", v1378);
		ex.input("1464", v1464);
		ex.extract("outout", denoised_uncond);
	}

	for (int c = 0; c < 4; c++) {
		float* u_ptr = denoised_uncond.channel(c);
		float* c_ptr = denoised_cond.channel(c);
		for (int hw = 0; hw < 32 * 32; hw++) {
			(*u_ptr) = (*u_ptr) + guidance_scale  * ((*c_ptr) - (*u_ptr));
			u_ptr++;
			c_ptr++;
		}
	}

	return denoised_uncond;
}

static vector<ncnn::Mat> saved_steps;

std::pair<ncnn::Mat, ncnn::Mat> DiffusionSlover::sampler_txt2img(int seed, int step, ncnn::Mat& c, ncnn::Mat& uc, const string& re_use_sentence)
{
	// t_to_sigma
	vector<float> sigma(step);
	float delta = 0.0 - 999.0 / (step - 1);
	for (int i = 0; i < step; i++) {
		float t = 999.0 + i * delta;
		int low_idx = floor(t);
		int high_idx = ceil(t);
		float w = t - low_idx;
		sigma[i] = exp((1 - w) * log_sigmas[low_idx] + w * log_sigmas[high_idx]);
	}
	sigma.push_back(0.f);

	// init
	ncnn::Mat x_mat = randn_4_32_32(seed % 1000);
	float _norm_[4] = { sigma[0], sigma[0], sigma[0], sigma[0] };
	x_mat.substract_mean_normalize(0, _norm_);
    ncnn::Mat denoise_x_mat;
	// sample_euler_ancestral
	{
		for (int i = 0; i < sigma.size() - 1; i++) {
            if (!re_use_sentence.empty()) {
                if (i < 2) {
                    __android_log_print(ANDROID_LOG_INFO, "SD", "Step:%2d skipped", i+1);
                    continue;
                } else if (i == 2) {
                    __android_log_print(ANDROID_LOG_INFO, "SD", "Step:%2d reused", i+1);
                    x_mat = latent_utils.get_denoised_item(re_use_sentence);
                    continue;
                }
            }

            double t1 = ncnn::get_current_time();
            ncnn::Mat denoised = CFGDenoiser_CompVisDenoiser(x_mat, sigma[i], c, uc);
			double t2 = ncnn::get_current_time();
			__android_log_print(ANDROID_LOG_INFO, "SD", "Step:%2d/%zu\t%fms", i+1, sigma.size()-1, t2-t1);



			float sigma_up = min(sigma[i + 1], sqrt(sigma[i + 1] * sigma[i + 1] * (sigma[i] * sigma[i] - sigma[i + 1] * sigma[i + 1]) / (sigma[i] * sigma[i])));
			float sigma_down = sqrt(sigma[i + 1] * sigma[i + 1] - sigma_up * sigma_up);

			srand(time(NULL) + 1);
			ncnn::Mat randn = randn_4_32_32(rand() % 1000);
			for (int c = 0; c < 4; c++) {
				float* x_ptr = x_mat.channel(c);
				float* d_ptr = denoised.channel(c);
				float* r_ptr = randn.channel(c);
				for (int hw = 0; hw < 32 * 32; hw++) {
					*x_ptr = *x_ptr + ((*x_ptr - *d_ptr) / sigma[i]) * (sigma_down - sigma[i]) + *r_ptr * sigma_up;
					x_ptr++;
					d_ptr++;
					r_ptr++;
				}
			}

            if (i == 2 and re_use_sentence.empty()) {
                denoise_x_mat = x_mat.clone();
            }
		}
	}

	ncnn::Mat fuck_x;
	fuck_x.clone_from(x_mat);
    return std::make_pair(fuck_x, denoise_x_mat);
//	return fuck_x;
}

ncnn::Mat DiffusionSlover::sampler_img2img(int seed, int step, ncnn::Mat& c, ncnn::Mat& uc, vector<ncnn::Mat>& init)
{
	// t_to_sigma
	vector<float> sigma(step);
	float delta = 0.0 - 999.0 / (step - 1);
	for (int i = 0; i < step; i++) {
		float t = 999.0 + i * delta;
		int low_idx = floor(t);
		int high_idx = ceil(t);
		float w = t - low_idx;
		sigma[i] = exp((1 - w) * log_sigmas[low_idx] + w * log_sigmas[high_idx]);
	}
	sigma.push_back(0.f);

	// init
	ncnn::Mat x_mat(32, 32, 4);

	// finish the rest of decoder
	{
		ncnn::Mat noise_mat = randn_4_32_32(seed % 1000);
		for (int c = 0; c < 4; c++) {
			float* x_ptr = x_mat.channel(c);
			float* noise_ptr = noise_mat.channel(c);
			float* mean_ptr = init[0].channel(c);
			float* std_ptr = init[1].channel(c);
			for (int hw = 0; hw < 32 * 32; hw++) {
				*x_ptr = *mean_ptr + *std_ptr * *noise_ptr;
				x_ptr++;
				noise_ptr++;
				mean_ptr++;
				std_ptr++;
			}
		}
		x_mat.substract_mean_normalize(0, factor);
	}

	// reset scheduling
	int new_step = step * strength;
	{
		float _sigma_ = sigma[step - new_step];
		ncnn::Mat noise_mat = randn_4_32_32(seed % 1000);
		for (int c = 0; c < 4; c++) {
			float* x_ptr = x_mat.channel(c);
			float* noise_ptr = noise_mat.channel(c);
			for (int hw = 0; hw < 32 * 32; hw++) {
				*x_ptr = *x_ptr + *noise_ptr * _sigma_;
				x_ptr++;
				noise_ptr++;
			}
		}
	}
	vector<float> sub_sigma(sigma.begin() + step - new_step, sigma.end());

	// euler ancestral
	{
		for (int i = 0; i < sub_sigma.size() - 1; i++) {
			double t1 = ncnn::get_current_time();
			ncnn::Mat denoised = CFGDenoiser_CompVisDenoiser(x_mat, sub_sigma[i], c, uc);
			double t2 = ncnn::get_current_time();
			__android_log_print(ANDROID_LOG_ERROR, "SD", "Step:%2d/%d\t%fms", i+1, sub_sigma.size()-1, t2-t1);

			float sigma_up = min(sub_sigma[i + 1], sqrt(sub_sigma[i + 1] * sub_sigma[i + 1] * (sub_sigma[i] * sub_sigma[i] - sub_sigma[i + 1] * sub_sigma[i + 1]) / (sub_sigma[i] * sub_sigma[i])));
			float sigma_down = sqrt(sub_sigma[i + 1] * sub_sigma[i + 1] - sigma_up * sigma_up);

			srand(time(NULL) + i);
			ncnn::Mat randn = randn_4_32_32(rand() % 1000);
			for (int c = 0; c < 4; c++) {
				float* x_ptr = x_mat.channel(c);
				float* d_ptr = denoised.channel(c);
				float* r_ptr = randn.channel(c);
				for (int hw = 0; hw < 32 * 32; hw++) {
					*x_ptr = *x_ptr + ((*x_ptr - *d_ptr) / sub_sigma[i]) * (sigma_down - sub_sigma[i]) + *r_ptr * sigma_up;
					x_ptr++;
					d_ptr++;
					r_ptr++;
				}
			}
		}
	}

	ncnn::Mat fuck_x;
	fuck_x.clone_from(x_mat);
	return fuck_x;
}
