/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include <QString>
#include <QStringList>
#include <vector>
#include <optional>
#include <memory>
#include "base/basic_types.h"

class HistoryItem;
struct TextWithTags;
class QVariant;

namespace Window {
class SessionController;
} // namespace Window

namespace Ui {
class Show;
class GenericBox;
} // namespace Ui

namespace PixelGram::LinkGuard {

enum class RiskLevel {
	Info,
	Low,
	Medium,
	High,
};

struct Flag {
	RiskLevel level = RiskLevel::Info;
	QString text;
};

struct Verdict {
	QString original;
	QString cleaned;
	QString finalUrl;
	QString host;
	QString displayHost;
	QString source; // "trusted", "unknown"
	std::optional<int> ageDays;
	bool isShortener = false;
	std::vector<QString> removedParams;
	std::vector<Flag> flags;

	[[nodiscard]] RiskLevel risk() const;
	[[nodiscard]] bool suspicious() const;
};

// URL cleaning
[[nodiscard]] QString CleanUrl(
	const QString &url,
	bool aggressive,
	std::vector<QString> *outRemoved = nullptr);
void CleanOutgoing(TextWithTags &textWithTags);

// URL analysis
[[nodiscard]] Verdict Analyze(
	const QString &url,
	const QString &anchor = QString());

// Link click interceptor
bool HandleUrlClick(
	const QString &url,
	const QVariant &context,
	std::shared_ptr<Ui::Show> show = nullptr);

bool IsBypassed(const QString &url);

// Context menu helpers
[[nodiscard]] std::vector<QString> ExtractUrls(not_null<HistoryItem*> item);
void ShowReportBox(
	not_null<Window::SessionController*> controller,
	const std::vector<QString> &urls);
void CopyCleanUrls(
	not_null<Window::SessionController*> controller,
	const std::vector<QString> &urls);

// Whitelist management
void AddWhitelistDomain(const QString &domain);
void RemoveWhitelistDomain(const QString &domain);
[[nodiscard]] bool IsWhitelisted(const QString &host);
[[nodiscard]] std::vector<QString> Whitelist();

// Database
void InitDatabase();
void RefreshDatabase(bool manual, Fn<void(bool success, int entries)> callback = nullptr);
[[nodiscard]] QString DatabaseStatus(bool full = false);

// Statistics
void ResetStats();
void IncrementCleaned(int count = 1);
void IncrementWarned();

} // namespace PixelGram::LinkGuard
