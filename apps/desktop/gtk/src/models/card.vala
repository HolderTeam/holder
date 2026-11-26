namespace HolderLinux {

public class CardSummary : Object {
    public string card_id { get; construct; }
    public string project_id { get; construct; }
    public string title { get; set; }
    public string rel_path { get; construct; }
    public double sort_key { get; set; }
    public string? parent_card_id { get; set; }
    public int64 created_at { get; construct; }
    public int64 updated_at { get; set; }

    public CardSummary(string card_id,
                       string project_id,
                       string title,
                       string rel_path,
                       double sort_key,
                       string? parent_card_id,
                       int64 created_at,
                       int64 updated_at) {
        Object(
            card_id: card_id,
            project_id: project_id,
            title: title,
            rel_path: rel_path,
            sort_key: sort_key,
            parent_card_id: parent_card_id,
            created_at: created_at,
            updated_at: updated_at
        );
    }
}

// A link, from a card or an AI message (from_card_id) to a card, AI message, AI thread or resource
// (to_card_id, of to_type), each in its project.
// One of the link kinds Holder knows, with how it reads from each end of a link.
public class LinkKindLabels : Object {
    public string kind { get; construct; }
    public string forward_label { get; construct; }
    public string reverse_label { get; construct; }

    public LinkKindLabels(string kind, string forward_label, string reverse_label) {
        Object(kind: kind, forward_label: forward_label, reverse_label: reverse_label);
    }
}

public class CardLink : Object {
    public string from_card_id { get; construct; }
    public string to_card_id { get; construct; }
    public string to_type { get; construct; }
    public string kind { get; construct; }
    public string? label { get; construct; }
    public int64 created_at { get; construct; }
    public string source_project_id { get; construct; }
    public string target_project_id { get; construct; }
    // What the link is from: "card" or "ai_message". Only a backlink can be from an AI message.
    public string source_type = "card";

    public CardLink(string from_card_id,
                    string to_card_id,
                    string to_type,
                    string kind,
                    string? label,
                    int64 created_at,
                    string source_project_id = "",
                    string target_project_id = "") {
        Object(
            from_card_id: from_card_id,
            to_card_id: to_card_id,
            to_type: to_type,
            kind: kind,
            label: label,
            created_at: created_at,
            source_project_id: source_project_id,
            target_project_id: target_project_id
        );
    }
}

public class CardDetail : Object {
    public string card_id { get; construct; }
    public string project_id { get; construct; }
    public string title { get; set; }
    public string content { get; set; }
    public int64 updated_at { get; set; }
    public string[] tags { get; private set; }
    public CardTagOccurrence[] tag_occurrences;

    public CardDetail(string card_id,
                      string project_id,
                      string title,
                      string content,
                      int64 updated_at,
                      string[]? tags = null,
                      CardTagOccurrence[]? tag_occurrences = null) {
        Object(
            card_id: card_id,
            project_id: project_id,
            title: title,
            content: content,
            updated_at: updated_at
        );
        this.tags = tags ?? new string[0];
        this.tag_occurrences = tag_occurrences ?? new CardTagOccurrence[0];
    }
}

// Which device made the latest change to one version, as this device's Git log tells it.
public enum ConflictEditedOn {
    THIS_DEVICE,
    ANOTHER_DEVICE,
    UNKNOWN
}

public class ConflictComparison : Object {
    public CardDetail original { get; construct; }
    public CardDetail copy_card { get; construct; }
    public string review_token { get; construct; }
    public int[] original_changed_lines;
    public int[] copy_changed_lines;
    public bool truncated { get; construct; }
    public ConflictEditedOn original_edited_on = ConflictEditedOn.UNKNOWN;
    public ConflictEditedOn copy_edited_on = ConflictEditedOn.UNKNOWN;

    public ConflictComparison(CardDetail original, CardDetail copy_card, int[] original_changed_lines,
                              int[] copy_changed_lines, bool truncated, string review_token) {
        Object(original: original, copy_card: copy_card, truncated: truncated, review_token: review_token);
        this.original_changed_lines = original_changed_lines;
        this.copy_changed_lines = copy_changed_lines;
    }
}

// A card the cleanup review names: a child of the copy, or where those children would move.
public class ConflictCleanupCard : Object {
    public string card_id { get; construct; }
    public string title { get; construct; }

    public ConflictCleanupCard(string card_id, string title) {
        Object(card_id: card_id, title: title);
    }
}

// One labelled date of a milestone, as the API gives it: a whole day or a moment.
public class ConflictMilestoneDate : Object {
    public string role { get; construct; }
    public string? date { get; construct; }
    public string? datetime { get; construct; }

    public ConflictMilestoneDate(string role, string? date, string? datetime) {
        Object(role: role, date: date, datetime: datetime);
    }
}

// A milestone only the copy has. Unlike the calendar's Milestone, it keeps every date, whatever
// its role, so nothing about it is lost before the user decides what to do with it.
public class ConflictCleanupMilestone : Object {
    public string milestone_id { get; construct; }
    public string? kind { get; construct; }
    public string? description { get; construct; }
    public ConflictMilestoneDate[] dates = {};

    public ConflictCleanupMilestone(string milestone_id, string? kind, string? description) {
        Object(milestone_id: milestone_id, kind: kind, description: description);
    }
}

public class ConflictCopyCleanupReview : Object {
    public string cleanup_token { get; construct; }
    // Where the copy's children would move; null for the project's top level.
    public ConflictCleanupCard? promotion_parent;
    public bool placement_differs;
    public bool preserves_original;
    public string[] copy_tags = {};
    public CardLink[] copy_links = {};
    public CardLink[] backlinks = {};
    public ConflictCleanupMilestone[] copy_milestones = {};
    public ConflictCleanupCard[] children = {};

    public ConflictCopyCleanupReview(string cleanup_token) {
        Object(cleanup_token: cleanup_token);
    }
}

// Which of the copy's own details, from its cleanup review, to carry over to the original before
// the copy goes to Trash. Anything left out stays with the copy.
public class ConflictCarryOver : Object {
    public CardLink[] links = {};
    public string[] milestone_ids = {};
    public string[] tags = {};
    public string[] child_ids = {};
    // Card links to the copy, to point at the original instead.
    public CardLink[] backlinks = {};

    public bool is_empty() {
        return links.length == 0 && milestone_ids.length == 0 && tags.length == 0 && child_ids.length == 0 &&
               backlinks.length == 0;
    }
}

public class ConflictResolutionResult : Object {
    public CardDetail card { get; construct; }
    public ConflictCopyCleanupReview cleanup { get; construct; }

    public ConflictResolutionResult(CardDetail card, ConflictCopyCleanupReview cleanup) {
        Object(card: card, cleanup: cleanup);
    }
}

public class CardTagOccurrence : Object {
    public string tag { get; construct; }
    public int byte_start { get; construct; }
    public int byte_end { get; construct; }

    public CardTagOccurrence(string tag, int byte_start, int byte_end) {
        Object(tag: tag, byte_start: byte_start, byte_end: byte_end);
    }
}

public class TagCount : Object {
    public string tag { get; construct; }
    public int card_count { get; construct; }

    public TagCount(string tag, int card_count) {
        Object(tag: tag, card_count: card_count);
    }
}

public class SearchCardResult : Object {
    public string card_id { get; construct; }
    public string title { get; construct; }
    public int64 updated_at { get; construct; }
    public int64 created_at { get; construct; }
    public string snippet { get; construct; }
    public double rank { get; construct; }

    public SearchCardResult(string card_id,
                            string title,
                            int64 updated_at,
                            int64 created_at,
                            string snippet,
                            double rank) {
        Object(
            card_id: card_id,
            title: title,
            updated_at: updated_at,
            created_at: created_at,
            snippet: snippet,
            rank: rank
        );
    }
}

public class CardMoveResult : Object {
    public string card_id { get; construct; }
    public string? parent_card_id { get; construct; }
    public double sort_key { get; construct; }

    public CardMoveResult(string card_id,
                          string? parent_card_id,
                          double sort_key) {
        Object(
            card_id: card_id,
            parent_card_id: parent_card_id,
            sort_key: sort_key
        );
    }
}

// A milestone as the calendar shows it: its start, and its end if it has one. Holder's
// milestones have any number of labelled dates; dates_json keeps them all, as the API gave
// them, so that an edit here changes only the start and end. One made elsewhere (the CLI's
// due=2026-12-01, say) can have no single start: it is shown by its earliest date whose role it
// holds once, start_at is that date, role names it and has_span is false.
public class Milestone : Object {
    public string milestone_id { get; construct; }
    public string project_id { get; set; default = ""; }
    public string? dates_json;
    public string card_id { get; construct; }
    public int64 start_at { get; construct; }
    public int64? end_at;
    public bool all_day { get; construct; }
    public string? kind { get; construct; }
    public string? description { get; construct; }
    public int64 created_at { get; construct; }
    public int64 updated_at { get; construct; }
    public string? card_title { get; construct; }
    // The role of the date start_at is: "start" unless the milestone has no single start.
    public string role = "start";
    // Whether end_at belongs to the shown date: only a start with at most one end has a span.
    public bool has_span = true;

    // Its kind, or else the role of a shown date other than its start ("Due"); null for neither.
    public string? display_kind() {
        if (kind != null && ((!) kind).strip().length > 0) return kind;
        return role == "start" || role.length == 0 ? null : role_words(role);
    }

    // A date role as words: "due" is "Due", "review_by" is "Review by".
    public static string role_words(string role) {
        var words = role.replace("_", " ");
        return words.length > 0 ? words.substring(0, 1).up() + words.substring(1) : "Date";
    }

    public Milestone(string milestone_id,
                     string card_id,
                     int64 start_at,
                     int64? end_at,
                     bool all_day,
                     string? kind,
                     string? description,
                     int64 created_at,
                     int64 updated_at,
                     string? card_title = null) {
        Object(
            milestone_id: milestone_id,
            card_id: card_id,
            start_at: start_at,
            all_day: all_day,
            kind: kind,
            description: description,
            created_at: created_at,
            updated_at: updated_at,
            card_title: card_title
        );
        this.end_at = end_at;
    }
}

public class CalendarCardActivity : Object {
    public string card_id { get; construct; }
    public string title { get; construct; }
    public int64 created_at { get; construct; }
    public int64 updated_at { get; construct; }

    public CalendarCardActivity(string card_id,
                                string title,
                                int64 created_at,
                                int64 updated_at) {
        Object(
            card_id: card_id,
            title: title,
            created_at: created_at,
            updated_at: updated_at
        );
    }
}

public class ProjectCalendar : Object {
    public string project_id { get; construct; }
    public int64 from_epoch { get; construct; }
    public int64 to_epoch { get; construct; }
    public Milestone[] milestones;
    public CalendarCardActivity[] created_cards;
    public CalendarCardActivity[] updated_cards;

    public ProjectCalendar(string project_id,
                           int64 from_epoch,
                           int64 to_epoch,
                           Milestone[] milestones,
                           CalendarCardActivity[] created_cards,
                           CalendarCardActivity[] updated_cards) {
        Object(
            project_id: project_id,
            from_epoch: from_epoch,
            to_epoch: to_epoch
        );
        this.milestones = milestones;
        this.created_cards = created_cards;
        this.updated_cards = updated_cards;
    }
}

public class CardHistorySave : Object {
    public string oid { get; construct; }
    public string[] parent_oids;
    public int64 authored_at { get; construct; }
    public int64 committed_at { get; construct; }
    public string message { get; construct; }

    public CardHistorySave(string oid,
                           string[] parent_oids,
                           int64 committed_at,
                           int64 authored_at = 0,
                           string message = "") {
        Object(
            oid: oid,
            authored_at: authored_at > 0 ? authored_at : committed_at,
            committed_at: committed_at,
            message: message
        );
        this.parent_oids = parent_oids;
    }
}

public class CardHistoryEntry : Object {
    public string first_oid { get; construct; }
    public string last_oid { get; construct; }
    public string[] parent_oids;
    // Direct parents that are also present in the current history page.
    public string[] visible_parent_oids;
    public string author_name { get; construct; }
    public string author_email { get; construct; }
    public int64 started_at { get; construct; }
    public int64 ended_at { get; construct; }
    public string kind { get; construct; }
    public string summary { get; construct; }
    public int commit_count { get; construct; }
    public bool is_merge { get; construct; }
    public CardHistorySave[] saves;

    public CardHistoryEntry(string first_oid,
                            string last_oid,
                            string[] parent_oids,
                            string author_name,
                            string author_email,
                            int64 started_at,
                            int64 ended_at,
                            string kind,
                            string summary,
                            int commit_count,
                            bool is_merge,
                            CardHistorySave[] saves = {},
                            string[] visible_parent_oids = {}) {
        Object(
            first_oid: first_oid,
            last_oid: last_oid,
            author_name: author_name,
            author_email: author_email,
            started_at: started_at,
            ended_at: ended_at,
            kind: kind,
            summary: summary,
            commit_count: commit_count,
            is_merge: is_merge
        );
        this.parent_oids = parent_oids;
        this.visible_parent_oids = visible_parent_oids;
        this.saves = saves;
    }
}

public class CardHistoryPage : Object {
    public string? head_oid { get; construct; }
    public CardHistoryEntry[] entries;
    public string? next_cursor { get; construct; }
    public bool scan_limited { get; construct; }

    public CardHistoryPage(string? head_oid,
                           CardHistoryEntry[] entries,
                           string? next_cursor,
                           bool scan_limited = false) {
        Object(head_oid: head_oid, next_cursor: next_cursor, scan_limited: scan_limited);
        this.entries = entries;
    }
}

public class CardHistoryVersion : Object {
    public bool exists { get; construct; }
    public string oid { get; construct; }
    public string title { get; construct; }
    public string body { get; construct; }

    public CardHistoryVersion(bool exists, string oid, string title, string body) {
        Object(exists: exists, oid: oid, title: title, body: body);
    }
}

public class CardHistoryDiffLine : Object {
    public string origin { get; construct; }
    public string text { get; construct; }
    public int64? old_line;
    public int64? new_line;

    public CardHistoryDiffLine(string origin, string text, int64? old_line, int64? new_line) {
        Object(origin: origin, text: text);
        this.old_line = old_line;
        this.new_line = new_line;
    }
}

public class CardHistoryComparison : Object {
    public CardHistoryVersion from_version { get; construct; }
    public CardHistoryVersion to_version { get; construct; }
    public string summary { get; construct; }
    public CardHistoryDiffLine[] lines;
    public bool truncated { get; construct; }

    public CardHistoryComparison(CardHistoryVersion from_version,
                                 CardHistoryVersion to_version,
                                 string summary,
                                 CardHistoryDiffLine[] lines,
                                 bool truncated) {
        Object(
            from_version: from_version,
            to_version: to_version,
            summary: summary,
            truncated: truncated
        );
        this.lines = lines;
    }
}

public class ProjectHistoryAffectedPath : Object {
    public string path { get; construct; }
    public string? title { get; construct; }
    public string? detail { get; construct; }
    public ProjectHistoryAffectedPath(string path, string? title = null, string? detail = null) {
        Object(path: path, title: title, detail: detail);
    }
}

public class ProjectHistoryAffectedObject : Object {
    public string kind { get; construct; }
    public ProjectHistoryAffectedPath[] items;
    public ProjectHistoryAffectedObject(string kind, ProjectHistoryAffectedPath[] items) {
        Object(kind: kind);
        this.items = items;
    }
}

public class ProjectHistoryActivity : Object {
    public string oid { get; construct; }
    public string[] parent_oids;
    public string author_name { get; construct; }
    public string author_email { get; construct; }
    public int64 committed_at { get; construct; }
    public string message { get; construct; }
    public ProjectHistoryAffectedObject[] affected_objects;
    public bool is_merge { get; construct; }
    public ProjectHistoryActivity(string oid, string[] parent_oids, string author_name,
                                  string author_email, int64 committed_at, string message,
                                  ProjectHistoryAffectedObject[] affected_objects, bool is_merge) {
        Object(oid: oid, author_name: author_name, author_email: author_email,
               committed_at: committed_at, message: message, is_merge: is_merge);
        this.parent_oids = parent_oids;
        this.affected_objects = affected_objects;
    }
}

public class ProjectHistoryPage : Object {
    public string? head_oid { get; construct; }
    public ProjectHistoryActivity[] activities;
    public string? next_cursor { get; construct; }
    public bool scan_limited { get; construct; }
    public ProjectHistoryPage(string? head_oid, ProjectHistoryActivity[] activities,
                              string? next_cursor, bool scan_limited = false) {
        Object(head_oid: head_oid, next_cursor: next_cursor, scan_limited: scan_limited);
        this.activities = activities;
    }
}

}
